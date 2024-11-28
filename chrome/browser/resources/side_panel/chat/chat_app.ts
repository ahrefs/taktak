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
    response: string,
}

export class ChatAppElement extends CrLitElement {
    private chatApiProxy_: ChatApiProxy = ChatApiProxyImpl.getInstance();
    private listenerIds_: number[] = [];
    protected actionList_: ActionItem[] = [];
    protected conversations_: conversationRecord[] = [];
    protected askAnythingLabel_ = loadTimeData.getString('askAnything');
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

    constructor() {
        super();
        ColorChangeUpdater.forDocument().start();
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
            submitResponse_: {type: Object},
            query_: {type: String},
            submittedQuery_: {type: String},
            completionResult_: {type: String},
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
        } else if (response.responseType == ResponseType.ERROR) {
            this.completionResult_ += "\n";
        }
    }

    protected onSubmitAction_(actionType: ActionType) {
        // todo: to handle type and display proper UI
        if (this.conversations_ != null) {
            if (actionType == ActionType.SUMMARIZE_PAGE) {
                this.conversations_.push({query: "Summarise this page.", response: ""});
            } else if (actionType == ActionType.EXPLAIN) {
                this.conversations_.push({query: "Explain this page in simple language.", response: ""});
            } else if (actionType == ActionType.FACT_CHECK) {
                this.conversations_.push({query: "Fact check this page.", response: ""});
            } else {
                this.conversations_.push({query: "Other actions. To be fixed.", response: ""});
            }
        }
        this.chatApiProxy_.submitAction(actionType);
    }

    protected onTextareaValueChanged_(e: CustomEvent<{ value: string }>) {
        this.query_ = e.detail.value;
    }

    protected async onSubmitQuery_() {
        if (this.completionResult_ && this.completionResult_.length > 0) {
            if (this.conversations_ != null) {
                if (this.conversations_.length == 0) {
                    this.conversations_.push({query: this.query_ ?? "", response: this.completionResult_});
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
        this.conversations_.push({query: this.query_ ?? "", response: ""});
        this.query_ = "";
        this.chatApiProxy_.submitQuery(ActionType.QUERY, this.submittedQuery_ ?? "");
    }

    override connectedCallback() {
        super.connectedCallback();

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