import {ChatAppElement} from "./chat_app";
import {html} from '//resources/lit/v3_0/lit.rollup.js';

export function getHtml(this: ChatAppElement) {
    return html`
        <div id="container">
            <div>
                <h5>${this.siteInfo_.url} </h5>
                <h6>${this.siteInfo_.title}</h6>
            </div>
            <div class="middle">
                ${this.submitResponse_.result}
            </div>
            <div class="bottom">
                ${this.actionList_.map((item,_) => html`
                        <cr-button class="action-button" @click="${this.onSubmitAction_}">
                            ${item.label}
                        </cr-button>
                `)}
                <div class="typing-textarea">
                    <textarea id="chat-input" spellcheck="false" placeholder="${this.askAnythingLabel_}" required></textarea>
                    <span id="send-btn" class="material-symbols-rounded">send</span>
                </div>
            </div>
        </div>`;
}