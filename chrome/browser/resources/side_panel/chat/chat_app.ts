import './strings.m.js';

import '//resources/cr_elements/cr_button/cr_button.js';
import '//resources/cr_elements/cr_icon_button/cr_icon_button.js';
import '//resources/cr_elements/cr_input/cr_input.js';
import '//resources/cr_elements/cr_textarea/cr_textarea.js';

import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';
import {getCss} from './chat_app.css.js';
import {getHtml} from './chat_app.html.js';
import type {ChatApiProxy} from "./chat_api_proxy.js";
import {ChatApiProxyImpl} from "./chat_api_proxy.js";
import {SiteInfo, ActionItem, ActionType, ActionResponse} from "./chat.mojom-webui.js";

export class ChatAppElement extends CrLitElement {
    private chatApiProxy_: ChatApiProxy = ChatApiProxyImpl.getInstance();
    private listenerIds_: number[] = [];
    protected actionList_: ActionItem[] = [];
    protected askAnythingLabel_ = loadTimeData.getString('askAnything');
    protected siteInfo_ : SiteInfo = {
        url: "",
        title: "",
        isContentUsableInConversations: false,
        isContentModified: false,
    };
    protected submitResponse_ : ActionResponse = {
        actionType: ActionType.NONE,
        result: ""
    };
    protected textareaValue_?: string;

    constructor() {
        super();
    }

    static get is() {
        return 'chat-app';
    }

    static override get styles() {
        return getCss();
    }

    static override get properties() {
        return {
            siteInfo_: {type: Object},
            askAnythingLabel_: {type: String},
            actionList_: {type: Array},
            submitResponse_: {type: Object},
            textareaValue_: {type: String},
        };
    }

    private async updateSiteInfo(siteInfo: SiteInfo) {
        this.siteInfo_ = siteInfo;
        if (this.siteInfo_.isContentUsableInConversations) {
            const {actionList}  = await this.chatApiProxy_.getActionList();
            this.actionList_ = actionList;
        }
        this.updateComplete;
    }

    private async updateSubmitResponse(response: ActionResponse) {
        this.submitResponse_ = response;
    }

    protected onSubmitAction_(e: Event) {
        e.stopPropagation();
        this.chatApiProxy_.submitAction(ActionType.SUMMARIZE_PAGE);
    }

    protected onTextareaValueChanged_(e: CustomEvent<{value: string}>) {
        this.textareaValue_ = e.detail.value;
    }

    protected async onSubmitQuery_() {
        console.log(this.textareaValue_);
        this.chatApiProxy_.submitQuery(ActionType.QUERY, this.textareaValue_ ?? "" );
    }

    override connectedCallback() {
        super.connectedCallback();

        setTimeout(() => this.chatApiProxy_.showUI(), 0);

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

    override render() {
        return getHtml.bind(this)();
    }
}

declare global {
    interface HTMLElementTagNameMap {
        'chat-app': ChatAppElement;
    }
}

customElements.define(ChatAppElement.is, ChatAppElement);