import {ChatAppElement} from "./chat_app";
import {html} from '//resources/lit/v3_0/lit.rollup.js';

export function getHtml(this: ChatAppElement) {
    return html`
        <div id="container">
            <div id="response-result" class="middle">
                ${
                        this.conversations_.map((conversation, _) => {
                            return conversation.query.length > 0
                                    ? html`
                                        <p class="query-container">${conversation.query}</p>
                                        ${conversation.response.length > 0 ? html`<p>
                                            ${conversation.response}</p>` : html``}`
                                    :
                                    html`${conversation.response.length > 0 ? html`<p>
                                        ${conversation.response}</p>` : html``}`

                        })
                }
                <p>
                    ${this.completionResult_}
                </p>
            </div>
            <div class="bottom">
                <div class="button-container">
                    ${this.siteInfo_.isContentUsableInConversations && this.conversations_ && this.conversations_.length == 0 ?
                            this.actionList_.map((item, _) => html`
                                <cr-button @click="${(e: Event) => {
                                    e.stopPropagation();
                                    this.onSubmitAction_(item.actionType);
                                }}" class="action-button">
                                    ${item.label}
                                </cr-button>
                            `) : html``}
                </div>
                <div class="typing-container">
                    <div class="site-info">
                        <p>${this.siteInfo_.url} </p>
                        <p>${this.siteInfo_.title}</p>
                    </div>
                    <div class="typing-content">
                        <cr-input
                                class="no-error"
                                type="text"
                                label=""
                                placeholder="Ask anything..."
                                .value="${this.query_}"
                                @value-changed="${this.onTextareaValueChanged_}">
                        </cr-input>
                        <cr-button @click="${this.onSubmitQuery_}">
                            Send
                        </cr-button>
                    </div>
                </div>
            </div>
        </div>`;
}