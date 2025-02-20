import './strings.m.js';
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
import {ActionItem, ActionResponse, ActionType, ResponseType, SiteInfo, ConversationItem} from "./chat.mojom-webui.js";
import type {ChatPromptInputElement} from "./chat_prompt_input";
import type {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';
import './chat_prompt_input.js';
import './action_menu.js';
import {marked} from "./marked.js";

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
    thinkingText: string,
    showThinkingText: boolean,
    responseText: string,
    errorText: string,
}


export interface ChatAppElement {
    $: {
        promptInput: ChatPromptInputElement,
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
    protected siteInfo_: SiteInfo = {
        url: "",
        title: "",
        isContentUsableInConversations: false,
    };
    protected query_?: string;
    protected submittedQuery_?: string;
    protected isSubmittingQuery_: boolean = false;
    protected shouldDisplayChatAboutThisPageButton_: boolean = false;
    protected exceedMaxTokenCountErrorMessages_: string = "";
    protected hasExceededMaxTokenCount_: boolean = false;
    protected maxPromptInputLength_: number = 90_000;
    protected shouldHideContextActionElementsInPromptInputDueToKnownContext_: boolean = false;
    protected shouldUseCurrentPageContentAsChatContext_: boolean = false;
    private isActivePageUrlNew_: boolean = false;
    private shouldHideSiteInfoInUserQueryElement_: boolean = false;
    protected shouldShowActionsMenu_: boolean = false;

    protected enableThinking_: boolean = false;
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
            askAnythingLabel_: {type: String},
            actionList_: {type: Array},
            translateToSubItems_: {type: String},
            socialMediaPostSubItems_: {type: String},
            shouldDisplayChatAboutThisPageButton_: {type: Boolean},
            exceedMaxLengthErrorMessages_: {type: String},
            hasExceededMaxTokenCount: {type: Boolean},
            maxPromptInputLength_: {type: Number},
            shouldUseCurrentPageContentAsChatContext_: {type: Boolean},
            shouldHideContextActionElementsInPromptInputDueToKnownContext_: {type: Boolean},
            isActivePageUrlNew_: {type: Boolean},
            shouldShowActionsMenu_: {type: Boolean},

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

    protected onThinkingToggleButtonClick_(e: Event) {
        e.preventDefault();
        this.enableThinking_ = !this.enableThinking_;
        console.log("enableThinking: " + this.enableThinking_);
    }

    protected onThinkingButtonClick_(id: string) {
        if (id === this.currentConversationId_) {
            this.showThinkingText_ = !this.showThinkingText_;
        } else {
            const index = this.conversations_.findIndex((conversation: ConversationRecord) => conversation.id === id);
            if (index >= 0 && index < this.conversations_.length) {
                const conversation = this.conversations_[index];
                if (conversation) {
                    this.conversations_ = [
                        ...this.conversations_.slice(0, index),
                        {
                            ...conversation,
                            showThinkingText: !conversation.showThinkingText,
                        },
                        ...this.conversations_.slice(index + 1),
                    ];
                }
            }
        }
        setTimeout(() => this.$.promptInput.focusInput(), 0);
    }

    private async updateSiteInfo(siteInfo: SiteInfo) {
        this.shouldShowActionsMenu_ = siteInfo.isContentUsableInConversations;
        if (this.siteInfo_.url === siteInfo.url) {
            this.isActivePageUrlNew_ = false;
            this.shouldHideSiteInfoInUserQueryElement_ = true;
            return;
        } else {
            this.isActivePageUrlNew_ = true;
            this.shouldHideSiteInfoInUserQueryElement_ = false;
        }

        this.siteInfo_ = siteInfo;
        if (this.siteInfo_.isContentUsableInConversations) {
            const {actionList} = await this.chatApiProxy_.getActionList();
            this.actionList_ = actionList;
            this.shouldUseCurrentPageContentAsChatContext_ = true;
            this.shouldDisplayChatAboutThisPageButton_ = false;
        } else {
            this.shouldUseCurrentPageContentAsChatContext_ = false;
        }
        this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
        this.updateComplete;
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
            }
        } else if (response.responseType == ResponseType.COMPLETED) {
            this.isThinking_ = false;
            this.currentResponseResult_ = this.removeCaret(this.currentResponseResult_) + "\n";
            this.isSubmittingQuery_ = false;
            setTimeout(() => this.$.promptInput.focusInput(), 0);
        } else if (response.responseType == ResponseType.ERROR) {
            this.isThinking_ = false;
            this.currentResponseResult_ = this.removeCaret(this.currentResponseResult_) + "\n";
            this.currentErrorResult_ = loadTimeData.getString('genericError');
            console.log("currentError_:" + this.currentErrorResult_);
            this.isSubmittingQuery_ = false;
            setTimeout(() => this.$.promptInput.focusInput(), 0);
        }
    }

    private logConversations() {
        for (const conversation of this.conversations_) {
            console.log("Start==============================");
            console.log("id: " + conversation.id);
            console.log("title: " + conversation.title);
            console.log("url: " + conversation.url);
            console.log("shouldDisplaySiteInfo: " + conversation.shouldDisplaySiteInfo);
            console.log("showThinking: " + conversation.showThinkingText);
            console.log("query: " + conversation.query);
            console.log("thinking: " + conversation.thinkingText);
            console.log("thinking parsed: " + marked.parse(conversation.thinkingText, {async: false}));
            console.log("response: " + conversation.responseText);
            console.log("response parsed: " + marked.parse(conversation.responseText, {async: false}));
            console.log("error: " + conversation.errorText);
            console.log("error parsed: " + marked.parse(conversation.errorText, {async: false}));
            console.log("End==============================");
        }
    }

    protected onCloseSidePanel_(e: Event) {
        e.preventDefault();
        this.storeCurrentConversation();
        this.logConversations();
        this.chatApiProxy_.closeUI();
    }

    protected onCancelQuery_() {
        this.query_ = "";
        this.submittedQuery_ = "";
        this.isSubmittingQuery_ = false;
        this.isThinking_ = false;
        this.$.promptInput.resetToAutoHeight();
        setTimeout(() => this.chatApiProxy_.cancelQuery(), 0);
        setTimeout(() => {
            this.currentResponseResult_ = this.removeCaret(this.currentResponseResult_);
            this.$.promptInput.focusInput();
        }, 300);
    }

    protected onRestartChat_(e: Event) {
        e.preventDefault();
        this.shouldDisplayChatAboutThisPageButton_ = false;
        this.hasExceededMaxTokenCount_ = false;
        this.exceedMaxTokenCountErrorMessages_ = "";
        this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
        this.shouldUseCurrentPageContentAsChatContext_ = this.siteInfo_.isContentUsableInConversations;
        this.shouldShowActionsMenu_ = this.siteInfo_.isContentUsableInConversations;

        this.conversations_.length = 0;
        this.query_ = "";
        this.isSubmittingQuery_ = false;
        this.submittedQuery_ = "";
        this.currentResponseResult_ = "";
        this.currentThinkingResult_ = "";
        this.isThinking_ = false;
        this.currentErrorResult_ = "";

        this.$.promptInput.resetToAutoHeight();
        this.$.promptInput.focusInput();
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

    private getCurrentConversation(): ConversationRecord | undefined {
        const lastIndex = this.conversations_.length - 1;
        if (lastIndex >= 0) {
            return this.conversations_[lastIndex];
        } else {
            return undefined;
        }
    }

    private storeCurrentConversation() {
        const lastConversation = this.getCurrentConversation();
        if (lastConversation) {
            lastConversation.responseText = this.currentResponseResult_;
            lastConversation.thinkingText = this.currentThinkingResult_;
            lastConversation.errorText = this.currentErrorResult_;
            lastConversation.showThinkingText = this.showThinkingText_;
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
            query: "",
            thinkingText: "",
            showThinkingText: true,
            responseText: "",
            errorText: "",
        };
    }

    protected onSubmitAction_(actionType: ActionType, actionParam: string = '') {
        this.storeCurrentConversation();

        const title = this.siteInfo_.title ?? "";
        const url = this.stripUrlProtocol_(this.siteInfo_.url ?? "");

        const currentConversation = this.getInitialConversation();
        currentConversation.title = title;
        currentConversation.url = url;
        currentConversation.shouldDisplaySiteInfo = true;

        if (actionType == ActionType.SUMMARIZE_PAGE) {
            this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
            this.shouldShowActionsMenu_ = false;
            currentConversation.query = loadTimeData.getString('promptSummarizeThisPage');
        } else if (actionType == ActionType.EXPLAIN) {
            this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
            this.shouldShowActionsMenu_ = false;
            currentConversation.query = loadTimeData.getString('promptExplainInSimpleLanguage');
        } else if (actionType == ActionType.FACT_CHECK) {
            this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
            this.shouldShowActionsMenu_ = false;
            currentConversation.query = loadTimeData.getString('promptFactCheck');
        } else if (actionType == ActionType.TRANSLATE) {
            this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
            this.shouldShowActionsMenu_ = false;
            currentConversation.query = loadTimeData.getString('promptTranslate') + ' ' + actionParam;
        } else if (actionType == ActionType.DRAFT_SOCIAL_MEDIA_POST) {
            this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
            this.shouldShowActionsMenu_ = false;
            currentConversation.query = loadTimeData.getString('promptSocialMediaPost') + ' ' + actionParam;
        }
        this.conversations_.push(currentConversation);
        this.isSubmittingQuery_ = true;
        setTimeout(() => this.chatApiProxy_.submitAction(actionType, actionParam, this.enableThinking_), 0);
    }

    protected onSubmitQuery_() {
        this.storeCurrentConversation();
        const currentConversation = this.getInitialConversation();

        currentConversation.query = this.query_ ?? "";
        currentConversation.title = this.siteInfo_.title ?? "";
        currentConversation.url = this.siteInfo_.url ?? "";
        currentConversation.shouldDisplaySiteInfo = this.isActivePageUrlNew_ && !this.shouldHideSiteInfoInUserQueryElement_;

        this.conversations_.push(currentConversation);

        this.submittedQuery_ = this.query_;
        this.query_ = "";
        this.$.promptInput.resetToAutoHeight();
        this.$.promptInput.focusInput();

        this.isSubmittingQuery_ = true;

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

        setTimeout(() =>
            this.chatApiProxy_.submitQuery(
                ActionType.QUERY,
                this.submittedQuery_ ?? "",
                this.shouldUseCurrentPageContentAsChatContext_ ? (this.siteInfo_.url || "") : "", conversation_history.reverse(), this.enableThinking_), 0);
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

    onLoad() {
        const updater = ColorChangeUpdater.forDocument();
        updater.start();
        updater.refreshColorsCss();
        this.$.promptInput.focusInput();
    }

    override connectedCallback() {
        super.connectedCallback();
        window.addEventListener('load', this.onLoad);
        setTimeout(async () => {
            this.chatApiProxy_.showUI();
            const {siteInfo} = await this.chatApiProxy_.getSiteInfo();
            await this.updateSiteInfo(siteInfo);
        }, 0);

        this.listenerIds_.push(
            this.chatApiProxy_.getCallbackRouter().onSiteInfoChanged.addListener(
                (siteInfo: SiteInfo) => this.updateSiteInfo(siteInfo)),
            this.chatApiProxy_.getCallbackRouter().onSubmitActionResponse.addListener(
                (response: ActionResponse) => this.updateCompletionResult(response))
        );
    }

    override disconnectedCallback() {
        super.disconnectedCallback();

        window.removeEventListener('load', this.onLoad);
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