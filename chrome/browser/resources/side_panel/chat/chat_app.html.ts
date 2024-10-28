import {ChatAppElement} from "./chat_app";
import {html} from '//resources/lit/v3_0/lit.rollup.js';

export function getHtml(this: ChatAppElement) {
    return html`
        <div id="container">
            <div>
                <h5>${this.siteInfo_.url} </h5>
                <h6>${this.siteInfo_.title}</h6>
            </div>
            <div class="middle"></div>
            <div class="bottom typing-textarea">
                <textarea id="chat-input" spellcheck="false" placeholder="${this.askAnythingLabel_}" required></textarea>
                <span id="send-btn" class="material-symbols-rounded">send</span>
            </div>
        </div>`;
}