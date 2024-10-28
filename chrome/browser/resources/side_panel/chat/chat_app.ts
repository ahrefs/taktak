import './strings.m.js';

import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';
import {getCss} from './chat_app.css.js';
import {getHtml} from './chat_app.html.js';
import type {ChatApiProxy} from "./chat_api_proxy.js";
import {ChatApiProxyImpl} from "./chat_api_proxy.js";
import {loadTimeData} from 'chrome://resources/js/load_time_data.js';

import {SiteInfo}
    from "./chat.mojom-webui.js";
export class ChatAppElement extends CrLitElement {
    private chatApiProxy_: ChatApiProxy = ChatApiProxyImpl.getInstance();


    private listenerIds_: number[] = [];
    protected siteInfo_ : SiteInfo = {
        url: "url",
        title: "title",
        isContentUsableInConversations: false,
        isContentModified: false,
    };

    protected askAnythingLabel_ = loadTimeData.getString('askAnything');

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
        };
    }



private async updateSiteInfo(siteInfo: SiteInfo) {
        this.siteInfo_ = siteInfo;
        await this.updateComplete;
    }

    override connectedCallback() {
        super.connectedCallback();
        setTimeout(() => this.chatApiProxy_.showUI(), 0);

        this.listenerIds_.push(
            this.chatApiProxy_.getCallbackRouter().onSiteInfoChanged.addListener(
                (siteInfo: SiteInfo) =>
                    this.updateSiteInfo(siteInfo)));

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