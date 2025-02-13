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
import {marked} from "./marked.js";
import type {ChatPromptInputElement} from "./chat_prompt_input";
import type {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';
import './chat_prompt_input.js';
import './action_menu.js';

function transformToArray(input: string): string[] {
    return input.split(",").map(item => item.trim());
}

export enum ActionOnExtractedContent {
    AddAsContext = 0,
    RemoveFromUsingAsContext = 1,
}

export enum ConversationRecordResponseType {
    EMPTY = 0,
    CONVERSATION = 1,
    ERROR = 2,
}

export type conversationRecord = {
    query: string,
    shouldDisplaySiteInfo: boolean,
    title: string,
    url: string,
    reasoning: string,
    response: string,
    responseType: ConversationRecordResponseType,
}

export interface ChatAppElement {
    $: {
        promptInput: ChatPromptInputElement,
    };
}

export class ChatAppElement extends CrLitElement {
    private chatApiProxy_: ChatApiProxy = ChatApiProxyImpl.getInstance();
    private listenerIds_: number[] = [];
    protected actionList_: ActionItem[] = [];
    protected translateToLanguages_: string[] = [];
    protected socialMediaPlatforms_: string[] = [];
    protected conversations_: conversationRecord[] = [];
    protected title_: string = loadTimeData.getString('title');
    protected askAnythingLabel_ = loadTimeData.getString('askAnything');
    protected chatAboutThisPageLabel_ = loadTimeData.getString('chatAboutThisPage');
    protected siteInfo_: SiteInfo = {
        url: "",
        title: "",
        isContentUsableInConversations: false,
    };
    protected completionResult_: string = "";
    protected query_?: string;
    protected submittedQuery_?: string;
    protected isSubmittingQuery_: boolean = false;
    protected shouldDisplayChatAboutThisPageButton_: boolean = false;
    protected exceedMaxTokenCountErrorMessages_: string = "";
    protected hasExceededMaxTokenCount_: boolean = false;
    protected errorMessage_: string = "";
    protected hasErrorOccurred_: boolean = false;
    protected maxPromptInputLength_: number = 90_000;
    protected shouldHideContextActionElementsInPromptInputDueToKnownContext_: boolean = false;
    protected shouldUseCurrentPageContentAsChatContext_: boolean = false;
    private isActivePageUrlNew_: boolean = false;
    private shouldHideSiteInfoInUserQueryElement_: boolean = false;
    protected shouldShowActionsMenu_: boolean = false;

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
            query_: {type: String},
            submittedQuery_: {type: String},
            completionResult_: {type: String},
            isSubmittingQuery_: {type: Boolean},
            shouldDisplayChatAboutThisPageButton_: {type: Boolean},
            exceedMaxLengthErrorMessages_: {type: String},
            hasExceededMaxTokenCount: {type: Boolean},
            errorMessage_: {type: String},
            hasErrorOccurred_: {type: Boolean},
            maxPromptInputLength_: {type: Number},
            shouldUseCurrentPageContentAsChatContext_: {type: Boolean},
            shouldHideContextActionElementsInPromptInputDueToKnownContext_: {type: Boolean},
            isActivePageUrlNew_: {type: Boolean},
            shouldShowActionsMenu_: {type: Boolean},
        };
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
            this.hasErrorOccurred_ = false;
            this.errorMessage_ = "";
        } else {
            this.shouldUseCurrentPageContentAsChatContext_ = false;
        }
        this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
        this.updateComplete;
    }

    private replaceThinkBlock(input: string) {
        const openingTagRegex = /<think>/g;
        const closingTagRegex = /<\/think>/g;

        let result = input.replace(openingTagRegex, '<blockquote>');
        return result.replace(closingTagRegex, '</blockquote>');
    }

    private removeCaret(input: string) {
        return input.replace(/<span class="caret"\/>/g, '');
    }

    private appendCaret(input: string) {
        return input + '<span class="caret"/>';
    }

    private updateCompletionResult(response: ActionResponse) {
        if (response.responseType == ResponseType.DELTA) {
            this.completionResult_ = this.removeCaret(this.completionResult_);
            this.completionResult_ += this.replaceThinkBlock(response.result);
            this.completionResult_ = this.appendCaret(this.completionResult_);
            this.hasErrorOccurred_ = false;
            this.errorMessage_ = "";
        } else if (response.responseType == ResponseType.COMPLETED) {
            this.completionResult_ = this.removeCaret(this.completionResult_);
            this.completionResult_ += "\n";
            this.isSubmittingQuery_ = false;
            this.hasErrorOccurred_ = false;
            this.errorMessage_ = "";
            this.$.promptInput.focusInput();
            setTimeout(() => this.$.promptInput.focusInput(), 0);
        } else if (response.responseType == ResponseType.ERROR) {
            this.completionResult_ = this.removeCaret(this.completionResult_);
            this.completionResult_ += "\n";
            this.isSubmittingQuery_ = false;
            this.hasErrorOccurred_ = true;
            this.errorMessage_ = loadTimeData.getString('genericError');
            this.$.promptInput.focusInput();
            setTimeout(() => this.$.promptInput.focusInput(), 0);
        }
    }

    private logConversations() {
        for (let i = 0; i < this.conversations_.length; i++) {
            const conversation = this.conversations_[i];
            if (conversation != undefined) {
                console.log("Start==============================");
                console.log("query: " + conversation.query);
                console.log("reasoning: " + conversation.reasoning);
                console.log("response: " + conversation.response);
                console.log("responseType: " + ConversationRecordResponseType[conversation.responseType]);
                console.log("shouldDisplaySiteInfo: " + conversation.shouldDisplaySiteInfo);
                console.log("title: " + conversation.title);
                console.log("url: " + conversation.url);
                console.log("End==============================");
            }
        }
    }

    protected onCloseSidePanel_(e: Event) {
        e.preventDefault();
        this.addLatestLLMResponseIntoLastConversation_();
        this.logConversations();
        this.chatApiProxy_.closeUI();
    }

    protected onCancelQuery_() {
        this.query_ = "";
        this.submittedQuery_ = "";
        this.isSubmittingQuery_ = false;
        this.$.promptInput.resetToAutoHeight();
        this.$.promptInput.focusInput();
        setTimeout(() => this.chatApiProxy_.cancelQuery(), 0);
        setTimeout(() => this.completionResult_ = this.removeCaret(this.completionResult_), 3);
    }

    protected onRestartChat_(e: Event) {
        e.preventDefault();
        this.query_ = "";
        this.conversations_.length = 0;
        this.completionResult_ = "";
        this.isSubmittingQuery_ = false;
        this.submittedQuery_ = "";
        this.shouldDisplayChatAboutThisPageButton_ = false;
        this.hasErrorOccurred_ = false;
        this.hasExceededMaxTokenCount_ = false;
        this.exceedMaxTokenCountErrorMessages_ = "";
        this.errorMessage_ = ""
        this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = false;
        this.shouldUseCurrentPageContentAsChatContext_ = this.siteInfo_.isContentUsableInConversations;
        this.shouldShowActionsMenu_ = this.siteInfo_.isContentUsableInConversations;
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

    private addLatestLLMResponseIntoLastConversation_() {
        const lastIndex = this.conversations_.length - 1;
        const lastConversation = this.conversations_[lastIndex];

        if (!lastConversation) return;

        if (this.completionResult_ && this.completionResult_.length > 0) {
            if (this.hasErrorOccurred_) {
                lastConversation.responseType = ConversationRecordResponseType.ERROR;
            } else {
                lastConversation.responseType = ConversationRecordResponseType.CONVERSATION;
                // todo: to check for reasoning type
                // Check the first <blockquote> block, even if it doesn’t have a closing </blockquote>
                // because user might manually stops the active conversation
                const match = this.completionResult_.match(/<blockquote>([\s\S]*?)(<\/blockquote>|$)/);
                if (match) {
                    let reasoningBlock = match[0];
                    const responseBlock = this.completionResult_.replace(reasoningBlock, "").trim();
                    if (!reasoningBlock.endsWith("</blockquote>")) {
                       reasoningBlock += "</blockquote>";
                    }
                    lastConversation.reasoning = reasoningBlock;
                    lastConversation.response = marked.parse(responseBlock, {async: false});
                }else {
                    lastConversation.response = marked.parse(this.completionResult_, {async: false});
                }
            }
            this.completionResult_ = "";
        } else {
            lastConversation.responseType = ConversationRecordResponseType.EMPTY
        }
    }

    protected onSubmitAction_(actionType: ActionType, actionParam: string = '') {
        this.addLatestLLMResponseIntoLastConversation_();
        const title = this.siteInfo_.title ?? "";
        const url = this.stripUrlProtocol_(this.siteInfo_.url ?? "");
        this.completionResult_ = "";
        const response = "";
        const reasoning = "";

        if (this.conversations_ != null) {
            if (actionType == ActionType.SUMMARIZE_PAGE) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.shouldShowActionsMenu_ = false;
                this.conversations_.push({
                    query: loadTimeData.getString('promptSummarizeThisPage'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    reasoning,
                    response,
                    responseType: ConversationRecordResponseType.EMPTY,
                });
            } else if (actionType == ActionType.EXPLAIN) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.shouldShowActionsMenu_ = false;
                this.conversations_.push({
                    query: loadTimeData.getString('promptExplainInSimpleLanguage'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    reasoning,
                    response,
                    responseType: ConversationRecordResponseType.EMPTY,
                });
            } else if (actionType == ActionType.FACT_CHECK) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.shouldShowActionsMenu_ = false;
                this.conversations_.push({
                    query: loadTimeData.getString('promptFactCheck'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    reasoning,
                    response,
                    responseType: ConversationRecordResponseType.EMPTY,
                });
            } else if (actionType == ActionType.TRANSLATE) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.shouldShowActionsMenu_ = false;
                this.conversations_.push({
                    query: loadTimeData.getString('promptTranslate') + ' ' + actionParam,
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    reasoning,
                    response,
                    responseType: ConversationRecordResponseType.EMPTY,
                });
            } else if (actionType == ActionType.DRAFT_SOCIAL_MEDIA_POST) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.shouldShowActionsMenu_ = false;
                this.conversations_.push({
                    query: loadTimeData.getString('promptSocialMediaPost') + ' ' + actionParam,
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    reasoning,
                    response,
                    responseType: ConversationRecordResponseType.EMPTY,
                });
            }
        }
        this.isSubmittingQuery_ = true;
        setTimeout(() => this.chatApiProxy_.submitAction(actionType, actionParam), 0);
    }

    protected onSubmitQuery_() {
        this.addLatestLLMResponseIntoLastConversation_();
        this.submittedQuery_ = this.query_;
        this.conversations_.push({
            query: this.query_ ?? "",
            shouldDisplaySiteInfo: this.isActivePageUrlNew_ && !this.shouldHideSiteInfoInUserQueryElement_,
            title: this.siteInfo_.title ?? "",
            url: this.siteInfo_.url ?? "",
            reasoning: "",
            response: "",
            responseType: ConversationRecordResponseType.EMPTY,
        });
        this.query_ = "";
        this.$.promptInput.resetToAutoHeight();
        this.$.promptInput.focusInput();

        this.isSubmittingQuery_ = true;

        // select the last 3 conversations to use as chat context
        const conversation_history: ConversationItem[] = [];
        for (let i = this.conversations_.length - 1; i >= 0; i--) {
            const conversation = this.conversations_[i];
            if (conversation != undefined && conversation.query.length > 0 && conversation.response.length > 0 && conversation_history.length <= 3) {
                conversation_history.push({
                    userQuery: conversation.query,
                    llmResponse: conversation.response,
                })
            }
        }

        setTimeout(() =>
            this.chatApiProxy_.submitQuery(
                ActionType.QUERY,
                this.submittedQuery_ ?? "",
                this.shouldUseCurrentPageContentAsChatContext_ ? (this.siteInfo_.url || "") : "", conversation_history.reverse()), 0);
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