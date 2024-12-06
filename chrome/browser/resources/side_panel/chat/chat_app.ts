import './strings.m.js';

import '//resources/cr_elements/cr_button/cr_button.js';
import '//resources/cr_elements/cr_icon_button/cr_icon_button.js';
import '//resources/cr_elements/cr_input/cr_input.js';
import '//resources/cr_elements/cr_textarea/cr_textarea.js';

import {ColorChangeUpdater} from 'chrome://resources/cr_components/color_change_listener/colors_css_updater.js';
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';
import {getCss} from './chat_app.css.js';
import {getHtml} from './chat_app.html.js';
import type {ChatApiProxy} from "./chat_api_proxy.js";
import {ChatApiProxyImpl} from "./chat_api_proxy.js";
import {ActionItem, ActionResponse, ActionType, ResponseType, SiteInfo} from "./chat.mojom-webui.js";

type conversationRecord = {
    query: string,
    shouldDisplaySiteInfo: boolean,
    title: string,
    url: string,
    response: string,
}

export class ChatAppElement extends CrLitElement {
    private chatApiProxy_: ChatApiProxy = ChatApiProxyImpl.getInstance();
    private listenerIds_: number[] = [];
    protected actionList_: ActionItem[] = [];
    protected conversations_: conversationRecord[] = [];
    protected askAnythingLabel_ = loadTimeData.getString('askAnything');
    protected chatAboutThisPageLabel_ = loadTimeData.getString('chatAboutThisPage');
    protected siteInfo_: SiteInfo = {
        url: "",
        title: "",
        isContentUsableInConversations: false,
    };
    protected submitResponse_: ActionResponse = {
        actionType: ActionType.NONE,
        responseType: ResponseType.NONE,
        result: "",
    };
    protected completionResult_: string = "";
    protected query_?: string;
    protected submittedQuery_?: string;
    protected promptForActionType_: string = "";
    protected isSubmittingQuery_: boolean = false;
    protected shouldDisplayChatAboutThisPageButton_: boolean = false;

    constructor() {
        super();
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
            chatAboutThisPage_: {type: String},
            actionList_: {type: Array},
            submitResponse_: {type: Object},
            query_: {type: String},
            submittedQuery_: {type: String},
            completionResult_: {type: String},
            promptForActionType_: {type: String},
            isSubmittingQuery_: {type: Boolean},
            shouldDisplayChatAboutThisPageButton_: {type: Boolean},
        };
    }

    private async updateSiteInfo(siteInfo: SiteInfo) {
        this.siteInfo_ = siteInfo;
        if (this.siteInfo_.isContentUsableInConversations) {
            const {actionList} = await this.chatApiProxy_.getActionList();
            this.actionList_ = actionList;
        }
        this.updateComplete;
    }

    private async updateSubmitResponse(response: ActionResponse) {
        // todo: to properly display error message
        if (response.responseType == ResponseType.DELTA) {
            this.completionResult_ += response.result;
        } else if (response.responseType == ResponseType.COMPLETED) {
            this.completionResult_ += "\n";
            this.isSubmittingQuery_ = false;
        } else if (response.responseType == ResponseType.ERROR) {
            this.completionResult_ += "\n";
            this.isSubmittingQuery_ = false;
        }
    }

    protected shouldDisplayChatAboutThisPageButton(value: boolean) {
        this.shouldDisplayChatAboutThisPageButton_ = value;
    }

    protected stripUrlProtocol(url: string = ''): string {
        const PROTOCOL_REGEX = /^https?:\/\//;
        return url ? url.replace(PROTOCOL_REGEX, '') : '';
    }

    protected onSubmitAction_(actionType: ActionType) {
        const title = this.siteInfo_.title ?? "";
        const url = this.stripUrlProtocol(this.siteInfo_.url ?? "");
        const response = "";
        if (this.conversations_ != null) {
            if (actionType == ActionType.SUMMARIZE_PAGE) {
                this.conversations_.push({
                    query: loadTimeData.getString('promptSummarizeThisPage'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else if (actionType == ActionType.EXPLAIN) {
                this.conversations_.push({
                    query: loadTimeData.getString('promptExplainInSimpleLanguage'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else if (actionType == ActionType.FACT_CHECK) {
                this.conversations_.push({
                    query: loadTimeData.getString('promptFactCheck'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else if (actionType == ActionType.TRANSLATE) {
                this.conversations_.push({
                    query: loadTimeData.getString('promptTranslate'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else if (actionType == ActionType.DRAFT_SOCIAL_MEDIA_POST) {
                this.conversations_.push({
                    query: loadTimeData.getString('promptSocialMediaPost'),
                    shouldDisplaySiteInfo: true,
                    title,
                    url,
                    response,
                });
            } else {
                // this branch should not be reached
                this.conversations_.push({
                    query: "", shouldDisplaySiteInfo: false,
                    title,
                    url,
                    response,
                });
            }
        }
        this.isSubmittingQuery_ = true;
        setTimeout(() => this.chatApiProxy_.submitAction(actionType), 0);
    }

    // To add it back once PromptInput component is finished
    // protected onTextareaValueChanged_(e: CustomEvent<{ value: string }>) {
    //     this.query_ = e.detail.value;
    // }

    protected onPromptInputChange_(e: Event){
        const target = e.target as HTMLInputElement;
        this.query_ = target.value;
        this.onSubmitQuery_().then();
    }

    protected async onSubmitQuery_() {
        if (this.completionResult_ && this.completionResult_.length > 0) {
            if (this.conversations_ != null) {
                if (this.conversations_.length == 0) {
                    this.conversations_.push({
                        query: this.query_ ?? "",
                        shouldDisplaySiteInfo: false,
                        title: this.siteInfo_.title ?? "",
                        url: this.siteInfo_.url ?? "",
                        response: this.completionResult_
                    });
                } else {
                    const lastIndex = this.conversations_.length - 1;
                    const lastConversation = this.conversations_[lastIndex];
                    if (lastConversation) {
                        lastConversation.response = this.completionResult_;
                    }
                }
            }
        }
        this.completionResult_ = "";
        this.submittedQuery_ = this.query_;
        this.conversations_.push({
            query: this.query_ ?? "",
            shouldDisplaySiteInfo: false,
            title: this.siteInfo_.title ?? "",
            url: this.siteInfo_.url ?? "",
            response: ""
        });
        this.query_ = "";

        this.isSubmittingQuery_ = true;
        setTimeout(() =>
            this.chatApiProxy_.submitQuery(ActionType.QUERY, this.submittedQuery_ ?? ""), 0);
    }

    refreshColorCss() {
        const updater = ColorChangeUpdater.forDocument();
        updater.start();
        updater.refreshColorsCss();
    }

    override connectedCallback() {
        super.connectedCallback();
        window.addEventListener('load', this.refreshColorCss);
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

        window.removeEventListener('load', this.refreshColorCss);
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