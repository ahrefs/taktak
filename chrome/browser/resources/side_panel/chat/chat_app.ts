// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '/strings.m.js';
import '//resources/cr_elements/cr_button/cr_button.js';
import '//resources/cr_elements/cr_icon_button/cr_icon_button.js';
import '//resources/cr_elements/cr_input/cr_input.js';
import '//resources/cr_elements/cr_textarea/cr_textarea.js';
import '//resources/cr_elements/cr_action_menu/cr_action_menu.js';
import '//resources/cr_elements/cr_dialog/cr_dialog.js';
import {ColorChangeUpdater} from 'chrome://resources/cr_components/color_change_listener/colors_css_updater.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';
import {getCss} from './chat_app.css.js';
import {getHtml} from './chat_app.html.js';
import type {ChatApiProxy} from "./chat_api_proxy.js";
import {ChatApiProxyImpl} from "./chat_api_proxy.js";
import {
    ActionItem,
    ActionResponse,
    ActionType,
    ResponseType,
    SiteInfo,
    ConversationItem,
    ChatState
} from "./chat.mojom-webui.js";
import type {ChatPromptInputElement} from "./chat_prompt_input";
import type {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';
import './chat_prompt_input.js';
import './action_menu.js';

export enum ActionOnExtractedContent {
    AddAsContext = 0,
    RemoveFromUsingAsContext = 1,
}

export type ConversationRecord = {
    id: string,
    query: string,
    title: string,
    url: string,
    shouldDisplaySiteInfo: boolean,
    isUrlContext: boolean,
    thinkingText: string,
    showThinkingText: boolean,
    responseText: string,
    errorText: string,
    timestamp: bigint,
}

export interface ChatAppElement {
    $: {
        promptInput: ChatPromptInputElement,
        conversationContainer: HTMLElement,
    };
}

function transformToArray(input: string): string[] {
    return input.split(",").map(item => item.trim());
}

export class ChatAppElement extends CrLitElement {
    private chatApiProxy_: ChatApiProxy = ChatApiProxyImpl.getInstance();
    private listenerIds_: number[] = [];
    protected actionList_: ActionItem[] = [];
    protected translateToLanguages_: string[] = [];
    protected socialMediaPlatforms_: string[] = [];
    protected conversations_: ConversationRecord[] = [];
    protected title_: string = loadTimeData.getString('title');
    protected askAnythingLabel_ = loadTimeData.getString('askAnything');
    protected chatAboutThisPageLabel_ = loadTimeData.getString('chatAboutThisPage');
    protected thinkingBtnLabel_ = loadTimeData.getString('thinking');
    protected doneThinkingBtnLabel_ = loadTimeData.getString('doneThinking');
    protected enableThinkingBtnLabel_ = loadTimeData.getString('enableThinking');
    protected thinkingEnabledBtnLabel_ = loadTimeData.getString('thinkingEnabled');

    protected maxPromptInputLength_: number = 90_000;

    protected siteInfo_: SiteInfo = {
        url: "",
        title: "",
        isContentUsableInConversations: false,
    };
    protected query_?: string;
    protected submittedQuery_?: string;
    protected isQuerySubmitting_: boolean = false;

    protected hasExceededMaxTokenCount_: boolean = false;
    protected exceedMaxTokenCountErrorMessages_: string = "";

    protected shouldDisplayChatAboutThisPageButton_: boolean = false;
    protected shouldHideContextActionElementsInPromptInputDueToKnownContext_: boolean = false;
    protected shouldUseCurrentPageContentAsChatContext_: boolean = false;
    private shouldHideSiteInfoInUserQueryElement_: boolean = false;
    protected shouldShowActionsMenu_: boolean = false;

    private isActivePageUrlNew_: boolean = false;
    protected enableThinking_: boolean = true;

    private shouldAutoScroll_: boolean = true;
    private scrollInterval_: number = 0;
    private scrollThreshold_: number = 0;
    private totalConversationLength_: number = 0;
    private conversationLengthThreshold_: number = 100_000;
    private isPointerDown_: boolean = false;

    // Individual properties are used to signal changes in the UI
    // instead of a single object. Using an object would result in
    // frequent allocations, which, over time, could degrade performance
    // and make the chat feature sluggish.
    protected isThinking_: boolean = false;
    protected currentResponseResult_: string = "";
    protected currentThinkingResult_: string = "";
    protected currentErrorResult_: string = "";
    protected currentConversationId_: string = "";
    protected showThinkingText_: boolean = true;

    private handleWheelOrTouchMove_ = this.onWheelOrTouchMove.bind(this);
    private handlePointerDown_ = this.onPointerDown.bind(this);
    private handlePointerUp_ = this.onPointerUp.bind(this);
    private handleConversationContainerScroll_ = this.onConversationContainerScroll.bind(this);

    constructor() {
        super();
        this.translateToLanguages_ = transformToArray(loadTimeData.getString('translateLanguages'));
        this.socialMediaPlatforms_ = transformToArray(loadTimeData.getString('socialMedias'));
    }

    static get is() {
        return 'chat-app';
    }

    static override get styles() {
        return getCss();
    }

    override render() {
        return getHtml.bind(this)();
    }

    static override get properties() {
        return {
            siteInfo_: {type: Object},
            actionList_: {type: Array},

            askAnythingLabel_: {type: String},
            translateToSubItems_: {type: String},
            socialMediaPostSubItems_: {type: String},
            enableThinkingBtnLabel_: {type: String},
            thinkingEnabledBtnLabel_: {type: String},
            exceedMaxLengthErrorMessages_: {type: String},

            hasExceededMaxTokenCount: {type: Boolean},
            maxPromptInputLength_: {type: Number},

            shouldDisplayChatAboutThisPageButton_: {type: Boolean},
            shouldUseCurrentPageContentAsChatContext_: {type: Boolean},
            shouldHideContextActionElementsInPromptInputDueToKnownContext_: {type: Boolean},
            shouldShowActionsMenu_: {type: Boolean},
            isActivePageUrlNew_: {type: Boolean},

            conversations_: {type: Array},
            query_: {type: String},
            submittedQuery_: {type: String},
            isSubmittingQuery_: {type: Boolean},
            isThinking_: {type: Boolean},
            currentResponseResult_: {type: String},
            currentThinkingResult_: {type: String},
            currentErrorResult_: {type: String},
            currentConversationId_: {type: String},
            showThinkingText_: {type: Boolean},
            enableThinking_: {type: Boolean},
        };
    }

    protected onToggleEnableThinking(e: Event) {
        e.preventDefault();
        this.enableThinking_ = !this.enableThinking_;
        this.chatApiProxy_.saveThinkingState(this.enableThinking_);
    }

    protected onThinkingButtonClick_(id: string) {
        if (id === this.currentConversationId_) {
            this.showThinkingText_ = !this.showThinkingText_;
        } else {
            const index = this.conversations_.findIndex((conversation: ConversationRecord) => conversation.id === id);
            if (index >= 0 && index < this.conversations_.length) {
                const conversation = this.conversations_[index];
                if (conversation) {
                    conversation.showThinkingText = !conversation.showThinkingText;
                    this.conversations_ = [
                        ...this.conversations_.slice(0, index),
                        conversation,
                        ...this.conversations_.slice(index + 1),
                    ];
                    this.chatApiProxy_.saveConversation(conversation);
                }
            }
        }
        setTimeout(() => this.$.promptInput.focusInput(), 0);
    }

    private async updateConversationHistory(chatState: ChatState) {
        this.conversations_ = chatState.conversations.sort((a, b) => Number(a.timestamp - b.timestamp));
        this.totalConversationLength_ = this.conversations_.reduce((acc, cur) => acc + cur.thinkingText.length + cur.responseText.length, 0);
        await this.updateComplete;
    }

    private removeCaret(input: string) {
        return input.replace(/<span class="caret"\/>/g, '');
    }

    private appendCaret(input: string) {
        return input + '<span class="caret"/>';
    }

    private updateCompletionResult(response: ActionResponse) {
        if (response.responseType == ResponseType.DELTA) {
            const responseResult = response.result;
            if (responseResult == "<think>") {
                this.isThinking_ = true;
            } else if (responseResult == "</think>") {
                this.isThinking_ = false;
            } else {
                if (this.isThinking_) {
                    this.currentThinkingResult_ += responseResult;
                } else {
                    this.currentResponseResult_ = this.removeCaret(this.currentResponseResult_);
                    this.currentResponseResult_ += responseResult;
                    // Prevent from showing caret while "thinking" is in progress
                    if (this.currentResponseResult_.length > 0) {
                        this.currentResponseResult_ = this.appendCaret(this.currentResponseResult_);
                    }
                }

                if (this.shouldAutoScroll_ && this.totalConversationLength_ < this.conversationLengthThreshold_) {
                    // To have an acceptable performance in the markdown container, we set thresholds for scrolling and word count.
                    if (this.totalConversationLength_ < 2_000) {
                        this.scrollThreshold_ = 0;
                    } else if (this.totalConversationLength_ < 4_000) {
                        this.scrollThreshold_ = 10;
                    } else if (this.totalConversationLength_ < 8_000) {
                        this.scrollThreshold_ = 16;
                    } else if (this.totalConversationLength_ < 30_000) {
                        this.scrollThreshold_ = 24;
                    } else if (this.totalConversationLength_ < 60_000) {
                        this.scrollThreshold_ = 32;
                    } else {
                        this.scrollThreshold_ = 64;
                    }

                    this.totalConversationLength_ += responseResult.length;

                    if (this.scrollInterval_ >= this.scrollThreshold_) {
                        this.$.conversationContainer.scrollTo({
                            top: this.$.conversationContainer.scrollHeight,
                            behavior: 'smooth'
                        });
                        this.scrollInterval_ = 0;
                    } else {
                        this.scrollInterval_++;
                    }
                }
            }
        } else if (response.responseType == ResponseType.COMPLETED) {
            this.isThinking_ = false;
            this.currentResponseResult_ = this.removeCaret(this.currentResponseResult_) + "\n";
            this.isQuerySubmitting_ = false;
            this.saveCurrentConversation();
            setTimeout(() => this.$.promptInput.focusInput(), 0);
            this.totalConversationLength_ = this.conversations_.reduce((acc, cur) => acc + cur.thinkingText.length + cur.responseText.length, 0);
            if (this.shouldAutoScroll_ && this.scrollInterval_ >= this.scrollThreshold_) {
                this.$.conversationContainer.scrollTo({
                    top: this.$.conversationContainer.scrollHeight,
                    behavior: 'instant'
                });
            }
            this.shouldAutoScroll_ = true;
        } else if (response.responseType == ResponseType.CHAT_RESPONSE_ERROR) {
            this.isThinking_ = false;
            this.currentResponseResult_ = this.removeCaret(this.currentResponseResult_) + "\n";
            this.currentErrorResult_ = loadTimeData.getString('genericError');
            this.isQuerySubmitting_ = false;
            this.shouldAutoScroll_ = true;
            this.saveCurrentConversation();
            setTimeout(() => this.$.promptInput.focusInput(), 0);
        }
    }

    protected onCloseSidePanel_(e: Event) {
        e.preventDefault();

        // Ensures that closing the side panel during an ongoing content extraction process
        // does not disrupt the operation and avoids potential pointer errors.
        // When the user attempts to close the panel mid-extraction, the closure is delayed
        // by 1 second. This delay allows the content extraction process to be initialized fully,
        // ensuring stability and preventing issues caused by prematurely nullified references.
        if (this.isQuerySubmitting_) {
            setTimeout(() => this.chatApiProxy_.closeUI(), 1000);
        } else {
            this.chatApiProxy_.closeUI();
        }
    }

    protected onCancelQuery_() {
        this.query_ = "";
        this.submittedQuery_ = "";
        this.isQuerySubmitting_ = false;
        this.isThinking_ = false;
        this.$.promptInput.resetToAutoHeight();
        setTimeout(() => this.chatApiProxy_.cancelQuery(), 0);
        setTimeout(() => {
            this.currentResponseResult_ = this.removeCaret(this.currentResponseResult_);
            this.saveCurrentConversation();
            this.$.promptInput.focusInput();
        }, 0);
    }

    protected onDeleteAll_(e: Event) {
        e.preventDefault();
        if (this.isQuerySubmitting_) {
            this.chatApiProxy_.cancelQuery();
        }
        this.conversations_ = [];
        this.isThinking_ = false;
        this.currentResponseResult_ = "";
        this.currentThinkingResult_ = "";
        this.currentErrorResult_ = "";

        // To make sure to clear active conversation text remained in memory
        setTimeout(() => {
            this.currentResponseResult_ = "";
            this.currentThinkingResult_ = "";
            this.currentErrorResult_ = "";
        });

        this.totalConversationLength_ = 0;

        this.hasExceededMaxTokenCount_ = false;
        this.exceedMaxTokenCountErrorMessages_ = "";

        this.shouldDisplayChatAboutThisPageButton_ = false;
        this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
        this.shouldUseCurrentPageContentAsChatContext_ = this.siteInfo_.isContentUsableInConversations;
        this.shouldShowActionsMenu_ = this.siteInfo_.isContentUsableInConversations;
        this.shouldHideSiteInfoInUserQueryElement_ = !this.siteInfo_.isContentUsableInConversations;

        this.query_ = "";
        this.isQuerySubmitting_ = false;
        this.submittedQuery_ = "";

        this.$.promptInput.resetToAutoHeight();
        this.$.promptInput.focusInput();

        this.chatApiProxy_.clearChatState();
    }

    protected onPerformActionOnExtractedContent_(action: ActionOnExtractedContent) {
        if (action === ActionOnExtractedContent.AddAsContext) {
            this.shouldDisplayChatAboutThisPageButton_ = false;
            this.shouldUseCurrentPageContentAsChatContext_ = true;
        } else if (action === ActionOnExtractedContent.RemoveFromUsingAsContext) {
            this.shouldDisplayChatAboutThisPageButton_ = true;
            this.shouldUseCurrentPageContentAsChatContext_ = false;
        }
        this.$.promptInput.focusInput();
        this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
    }

    protected stripUrlProtocol_(url: string = ''): string {
        const PROTOCOL_REGEX = /^https?:\/\//;
        return url ? url.replace(PROTOCOL_REGEX, '') : '';
    }

    protected onActionMenuItemClick_(e: CustomEvent<{ actionType: ActionType, actionParam: string }>) {
        this.onSubmitAction_(e.detail.actionType, e.detail.actionParam);
    }

    private saveCurrentConversation() {
        const lastIndex = this.conversations_.length - 1;
        const currentConversation = this.conversations_[lastIndex];
        if (currentConversation) {
            currentConversation.responseText = this.currentResponseResult_;
            currentConversation.thinkingText = this.currentThinkingResult_;
            currentConversation.errorText = this.currentErrorResult_;
            currentConversation.showThinkingText = this.showThinkingText_;
            setTimeout(() =>
                this.chatApiProxy_.saveConversation({
                    ...currentConversation,
                }), 0);
        }
        this.currentConversationId_ = "";
        this.currentResponseResult_ = "";
        this.currentThinkingResult_ = "";
        this.currentErrorResult_ = "";
        this.showThinkingText_ = true;
    }

    private getInitialConversation(): ConversationRecord {
        this.currentConversationId_ = crypto.randomUUID();
        return {
            id: this.currentConversationId_,
            title: "",
            url: "",
            shouldDisplaySiteInfo: false,
            isUrlContext: false,
            query: "",
            thinkingText: "",
            showThinkingText: true,
            responseText: "",
            errorText: "",
            timestamp: BigInt(Date.now()),
        };
    }

    protected onSubmitAction_(actionType: ActionType, actionParam: string = '') {
        this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
        this.shouldShowActionsMenu_ = false;
        this.isQuerySubmitting_ = true;

        const title = this.siteInfo_.title ?? "";
        const url = this.stripUrlProtocol_(this.siteInfo_.url ?? "");

        const currentConversation = this.getInitialConversation();
        currentConversation.title = title;
        currentConversation.url = url;
        currentConversation.shouldDisplaySiteInfo = true;
        currentConversation.isUrlContext = true;

        if (actionType == ActionType.SUMMARIZE_PAGE) {
            currentConversation.query = loadTimeData.getString('promptSummarizeThisPage');
        } else if (actionType == ActionType.EXPLAIN) {
            currentConversation.query = loadTimeData.getString('promptExplainInSimpleLanguage');
        } else if (actionType == ActionType.FACT_CHECK) {
            currentConversation.query = loadTimeData.getString('promptFactCheck');
        } else if (actionType == ActionType.TRANSLATE) {
            currentConversation.query = loadTimeData.getString('promptTranslate') + ' ' + actionParam;
        } else if (actionType == ActionType.DRAFT_SOCIAL_MEDIA_POST) {
            currentConversation.query = loadTimeData.getString('promptSocialMediaPost') + ' ' + actionParam;
        }
        this.conversations_.push(currentConversation);

        this.shouldAutoScroll_ = true;
        this.$.conversationContainer.scrollTo({
            top: this.$.conversationContainer.scrollHeight,
            behavior: 'smooth'
        });
        setTimeout(() => {
            this.chatApiProxy_.submitAction(actionType, actionParam, this.enableThinking_)
        }, 0);
    }

    protected onSubmitQuery_() {
        this.shouldShowActionsMenu_ = false;
        const currentConversation = this.getInitialConversation();

        currentConversation.query = this.query_ ?? "";
        currentConversation.title = this.siteInfo_.title ?? "";
        currentConversation.url = this.stripUrlProtocol_(this.siteInfo_.url ?? "");
        currentConversation.isUrlContext = this.shouldUseCurrentPageContentAsChatContext_;

        const lastIndex = this.conversations_.length - 1;
        const lastConversation = this.conversations_[lastIndex];
        if (lastConversation) {
            currentConversation.shouldDisplaySiteInfo = (lastConversation.url === currentConversation.url &&
                lastConversation.title === currentConversation.title && lastConversation.isUrlContext)
                ? false
                : this.shouldUseCurrentPageContentAsChatContext_;
        } else {
            currentConversation.shouldDisplaySiteInfo =
                (this.isActivePageUrlNew_ &&
                    !this.shouldHideSiteInfoInUserQueryElement_ &&
                    this.shouldUseCurrentPageContentAsChatContext_)
                ||
                /* This is for the situation where the side panel is closed while there is an active streaming based on the content of the active page.
                   In that case, the active conversation will not be saved into cache due to the abrupt closure.
                 */
                (!this.isActivePageUrlNew_ && this.siteInfo_.isContentUsableInConversations &&
                    this.conversations_.length == 0 &&
                    this.shouldUseCurrentPageContentAsChatContext_);
        }

        this.conversations_.push(currentConversation);

        this.submittedQuery_ = this.query_;
        this.query_ = "";
        this.$.promptInput.resetToAutoHeight();
        this.$.promptInput.focusInput();

        this.isQuerySubmitting_ = true;

        // Retrieve the last 3 conversations to provide chat context, omitting any reasoning text.
        const conversation_history: ConversationItem[] = [];
        for (let i = this.conversations_.length - 1; i >= 0; i--) {
            const conversation = this.conversations_[i];
            if (conversation != undefined && conversation.query.length > 0 && conversation.responseText.length > 0 && conversation_history.length <= 3) {
                conversation_history.push({
                    userQuery: conversation.query,
                    llmResponse: conversation.responseText,
                })
            }
        }

        this.shouldAutoScroll_ = true;
        this.$.conversationContainer.scrollTo({
            top: this.$.conversationContainer.scrollHeight,
            behavior: 'instant'
        });

        setTimeout(() => {
            this.chatApiProxy_.submitQuery(
                ActionType.QUERY,
                this.submittedQuery_ ?? "",
                this.shouldUseCurrentPageContentAsChatContext_ ? (this.siteInfo_.url || "") : "", conversation_history.reverse(), this.enableThinking_)
        }, 0);
    }

    protected onPromptInputChange_(e: CustomEvent<{ value: string }>) {
        this.query_ = e.detail.value;

        if (this.query_.length > this.maxPromptInputLength_) {
            this.exceedMaxTokenCountErrorMessages_ = loadTimeData.getString("promptExceedMaxTokenCount") + " - " + this.query_.length + "/" + this.maxPromptInputLength_ + ".";
            this.hasExceededMaxTokenCount_ = true;
        } else {
            this.exceedMaxTokenCountErrorMessages_ = "";
            this.hasExceededMaxTokenCount_ = false;
        }
    }

    protected onSetAndSubmitQuery_(e: CustomEvent<{ value: string }>) {
        this.query_ = e.detail.value;
        if (this.query_ !== "" && !this.hasExceededMaxTokenCount_) {
            this.onSubmitQuery_();
        }

        // hide site info from prompt input because it's obvious that the content of currently opening site will be used as context
        if ((this.siteInfo_.url != undefined && this.siteInfo_.url.length > 0) && this.siteInfo_.isContentUsableInConversations) {
            this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
            this.shouldHideSiteInfoInUserQueryElement_ = true;
        } else {
            this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
            this.shouldHideSiteInfoInUserQueryElement_ = false;
        }
    }

    protected openUrl_(url: string, modifiers: ClickModifiers) {
        this.chatApiProxy_.openUrl(url, modifiers);
    }

    private async updateSiteInfo(siteInfo: SiteInfo, willSaveInCache: boolean) {
        if (willSaveInCache) {
            this.chatApiProxy_.saveSiteInfo(siteInfo);
        }
        if (this.siteInfo_.url === siteInfo.url) {
            const lastConversation = this.conversations_[this.conversations_.length - 1];
            if (lastConversation) {
                if (lastConversation.url == this.stripUrlProtocol_(siteInfo.url ?? "")) {
                    this.isActivePageUrlNew_ = false;
                    this.shouldHideSiteInfoInUserQueryElement_ = lastConversation.isUrlContext;
                    this.shouldShowActionsMenu_ = !lastConversation.isUrlContext;
                    this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = lastConversation.isUrlContext;
                } else {
                    this.isActivePageUrlNew_ = true;
                    this.shouldHideSiteInfoInUserQueryElement_ = false;
                    this.shouldShowActionsMenu_ = true;
                    this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
                }
            } else {
                this.isActivePageUrlNew_ = false;
                this.shouldHideSiteInfoInUserQueryElement_ = false;
                this.shouldShowActionsMenu_ = true;
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
            }
        } else {
            const lastConversation = this.conversations_[this.conversations_.length - 1];
            /*
             This `if` condition evaluates the following scenarios:
             1. The user visits abc.com, where the content can be utilized as chat context,
                and they interact with it by asking a related question.
             2. Afterward, the user navigates to other websites, but user don't use these sites' content
                as the chat context.
             3. Finally, the user navigates back to abc.com, potentially resuming the context
                for further questions.

                In this scenario, the known context is still the content of abc.com,
                so suggestion list and site info in the prompt input container will not be displayed.
                But the content of the abc.com will be used as the context of the chat.
             */
            if (lastConversation && lastConversation.isUrlContext && lastConversation.url == this.stripUrlProtocol_(siteInfo.url ?? "")) {
                this.shouldShowActionsMenu_ = false;
                this.shouldHideSiteInfoInUserQueryElement_ = true;
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
            } else {
                this.isActivePageUrlNew_ = true;
                this.shouldHideSiteInfoInUserQueryElement_ = false;
                this.shouldShowActionsMenu_ = siteInfo.isContentUsableInConversations;
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
            }
        }

        this.siteInfo_ = siteInfo;
        if (this.siteInfo_.isContentUsableInConversations) {
            const {actionList} = await this.chatApiProxy_.getActionList();
            this.actionList_ = actionList;
            this.shouldUseCurrentPageContentAsChatContext_ = true;
            this.shouldDisplayChatAboutThisPageButton_ = false;
        } else {
            this.shouldUseCurrentPageContentAsChatContext_ = false;
            this.shouldShowActionsMenu_ = false;
        }

        // Lit requires this to update
        await this.updateComplete;
    }

    onLoad() {
        const updater = ColorChangeUpdater.forDocument();
        updater.start();
        updater.refreshColorsCss();
    }

    onWheelOrTouchMove() {
        this.shouldAutoScroll_ = false;
    }

    onPointerDown() {
        this.isPointerDown_ = true
    }

    onPointerUp() {
        this.isPointerDown_ = false;
    }

    onConversationContainerScroll() {
        if (this.isPointerDown_) {
            this.shouldAutoScroll_ = false;
        }
    }

    override connectedCallback() {
        super.connectedCallback();

        window.addEventListener('load', this.onLoad);

        const conversationContainer = this.shadowRoot?.querySelector('#conversationContainer') as HTMLElement;
        if (conversationContainer) {
            conversationContainer.addEventListener('wheel', this.handleWheelOrTouchMove_);
            conversationContainer.addEventListener('touchmove', this.handleWheelOrTouchMove_);
            conversationContainer.addEventListener('pointerdown', this.handlePointerDown_);
            conversationContainer.addEventListener('pointerup', this.handlePointerUp_);
            conversationContainer.addEventListener('scroll', this.handleConversationContainerScroll_);
        }

        setTimeout(async () => {
            this.chatApiProxy_.showUI();
            const {chatState} = await this.chatApiProxy_.getChatState();
            await this.updateConversationHistory(chatState);

            const {thinkingState} = await this.chatApiProxy_.getThinkingState();
            this.enableThinking_ = thinkingState;

            let cacheSiteInfo = await this.chatApiProxy_.getSiteInfoFromCache();
            await this.updateSiteInfo(cacheSiteInfo.siteInfo, false /* willSaveInCache */);

            let {siteInfo} = await this.chatApiProxy_.getSiteInfo();
            await this.updateSiteInfo(siteInfo, true /* willSaveInCache */);
        }, 0);

        this.listenerIds_.push(
            this.chatApiProxy_.getCallbackRouter().onSiteInfoChanged.addListener(
                (siteInfo: SiteInfo) => this.updateSiteInfo(siteInfo, true /* willSaveInCache */)),
            this.chatApiProxy_.getCallbackRouter().onSubmitActionResponse.addListener(
                (response: ActionResponse) => this.updateCompletionResult(response))
        );
    }

    override disconnectedCallback() {
        super.disconnectedCallback();

        window.removeEventListener('load', this.onLoad);

        const conversationContainer = this.shadowRoot.querySelector('#conversationContainer') as HTMLElement;
        if (conversationContainer) {
            conversationContainer.removeEventListener('wheel', this.handleWheelOrTouchMove_);
            conversationContainer.removeEventListener('touchmove', this.handleWheelOrTouchMove_);
            conversationContainer.removeEventListener('pointerdown', this.handlePointerDown_);
            conversationContainer.removeEventListener('pointerup', this.handlePointerUp_);
            conversationContainer.removeEventListener('scroll', this.handleConversationContainerScroll_);
        }

        this.listenerIds_.forEach(
            id => this.chatApiProxy_.getCallbackRouter().removeListener(id));
    }
}

declare global {
    interface HTMLElementTagNameMap {
        'chat-app': ChatAppElement;
    }
}

customElements.define(ChatAppElement.is, ChatAppElement);