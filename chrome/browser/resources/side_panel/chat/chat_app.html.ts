import {ChatAppElement} from "./chat_app";
import {html} from '//resources/lit/v3_0/lit.rollup.js';

export function getHtml(this: ChatAppElement) {
    return html`
        <div id="container">
            <div class="middle">
                ${this.submitResponse_.result}
            </div>
            <div class="bottom">
                <div class="button-container">
                ${this.actionList_.map((item,_) => html`
                        <cr-button @click="${this.onSubmitAction_}">
                            ${item.label}
                        </cr-button>
                `)}
                </div>
                <div class="typing-container">
                    <cr-input
                            class="no-error"
                            type="text"
                            label=""
                            placeholder="Ask anything..."
                            .value="${this.textareaValue_}"
                            @value-changed="${this.onTextareaValueChanged_}">
                    </cr-input>
                        <cr-button @click="${this.onSubmitQuery_}">
                            Send
                        </cr-button>
                </div>
            </div>
        </div>`;
}