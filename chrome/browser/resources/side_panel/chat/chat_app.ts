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

export type conversationRecord = {
    query: string,
    shouldDisplaySiteInfo: boolean,
    title: string,
    url: string,
    response: string,
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
            shouldHideSiteInfoContainerDueToKnownContext_: {type: Boolean},
            shouldUseCurrentPageContentAsChatContext_: {type: Boolean},
            title_: {type: String},
            isActivePageUrlNew_: {type: Boolean},
        };
    }

    private async updateSiteInfo(siteInfo: SiteInfo) {
        if (siteInfo !== undefined && siteInfo.url != undefined && siteInfo.url.length == 0) {
            return;
        }

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

    private updateSubmitResponse(response: ActionResponse) {
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
            setTimeout(() => this.$.promptInput.focusInput(), 0);
        } else if (response.responseType == ResponseType.ERROR) {
            this.completionResult_ += "\n";
            this.isSubmittingQuery_ = false;
            this.hasErrorOccurred_ = true;
            this.errorMessage_ = loadTimeData.getString('genericError');
            setTimeout(() => this.$.promptInput.focusInput(), 0);
        }
    }

    protected onCloseSidePanel_(e: Event) {
        e.preventDefault();
        this.chatApiProxy_.closeUI();
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
        if (this.siteInfo_.isContentUsableInConversations) {
            this.shouldUseCurrentPageContentAsChatContext_ = true;
        } else {
            this.shouldUseCurrentPageContentAsChatContext_ = false;
        }
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

    protected onSubmitAction_(actionType: ActionType, actionParam: string = '') {
        const title = this.siteInfo_.title ?? "";
        const url = this.stripUrlProtocol_(this.siteInfo_.url ?? "");
        const response = "";
        if (this.conversations_ != null) {
            if (actionType == ActionType.SUMMARIZE_PAGE) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.conversations_.push({
                    query: loadTimeData.getString('promptSummarizeThisPage'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else if (actionType == ActionType.EXPLAIN) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.conversations_.push({
                    query: loadTimeData.getString('promptExplainInSimpleLanguage'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else if (actionType == ActionType.FACT_CHECK) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.conversations_.push({
                    query: loadTimeData.getString('promptFactCheck'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else if (actionType == ActionType.TRANSLATE) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.conversations_.push({
                    query: loadTimeData.getString('promptTranslate') + ' ' + actionParam,
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else if (actionType == ActionType.DRAFT_SOCIAL_MEDIA_POST) {
                this.shouldHideContextActionElementsInPromptInputDueToKnownContext_ = true;
                this.conversations_.push({
                    query: loadTimeData.getString('promptSocialMediaPost') + ' ' + actionParam,
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else {
                // this branch should not be reached
                this.conversations_.push({
                    query: "",
                    shouldDisplaySiteInfo: false,
                    title,
                    url,
                    response,
                });
            }
        }
        this.isSubmittingQuery_ = true;
        setTimeout(() => this.chatApiProxy_.submitAction(actionType, actionParam), 0);
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

    protected onSubmitQuery_() {
        if (this.completionResult_ && this.completionResult_.length > 0) {
            if (this.conversations_ != null) {
                if (this.conversations_.length == 0) {
                    this.conversations_.push({
                        query: this.query_ ?? "",
                        shouldDisplaySiteInfo: this.isActivePageUrlNew_ && !this.shouldHideSiteInfoInUserQueryElement_,
                        title: this.siteInfo_.title ?? "",
                        url: this.siteInfo_.url ?? "",
                        response: marked.parse(this.completionResult_, {async: false}),
                    });
                } else {
                    const lastIndex = this.conversations_.length - 1;
                    const lastConversation = this.conversations_[lastIndex];
                    if (lastConversation) {
                        lastConversation.response = marked.parse(this.completionResult_, {async: false});
                    }
                }
            }
        }
        this.completionResult_ = "";
        this.submittedQuery_ = this.query_;
        this.conversations_.push({
            query: this.query_ ?? "",
            shouldDisplaySiteInfo: this.isActivePageUrlNew_ && !this.shouldHideSiteInfoInUserQueryElement_,
            title: this.siteInfo_.title ?? "",
            url: this.siteInfo_.url ?? "",
            response: ""
        });
        this.query_ = "";
        this.$.promptInput.resetToAutoHeight();
        this.$.promptInput.focusInput();

        this.isSubmittingQuery_ = true;

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

    protected onCancelQuery_() {
        this.query_ = "";
        this.submittedQuery_ = "";
        this.isSubmittingQuery_ = false;
        this.$.promptInput.resetToAutoHeight();
        this.$.promptInput.focusInput();
        setTimeout(() => this.chatApiProxy_.cancelQuery(), 0);
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
                (response: ActionResponse) => this.updateSubmitResponse(response))
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