import {ChatAppElement} from "./chat_app";
import {html} from '//resources/lit/v3_0/lit.rollup.js';

export function getHtml(this: ChatAppElement) {
    return html`
        <div id="container">
            <div id="conversation-container">
                <div class="conversation-content">
                    ${
                            this.conversations_.map((conversation, _) => {
                                return conversation.query.length > 0
                                        ? html`
                                            ${conversation.shouldDisplaySiteInfo ? html`
                                                <div class="query-prompt-container auto-width">
                                                    <div class="siteinfo-container">
                                                        <div class="vertical-bar"></div>
                                                        <div class="siteinfo-content">
                                                            <div class="siteinfo-title">${conversation.title}</div>
                                                            <div class="siteinfo-url">${conversation.url}</div>
                                                        </div>
                                                    </div>
                                                    <div class="prompt-section">${conversation.query}</div>
                                                </div>` : html`
                                                <div class="query-prompt-container content-fit-width">
                                                    <div class="prompt-section">${conversation.query}</div>
                                                </div>`
                                            }
                                            ${conversation.response.length > 0 ? html`
                                                <div class="message-container">
                                                    ${conversation.response}
                                                </div>` : html``}`
                                        :
                                        html`${conversation.response.length > 0 ? html`
                                            <div class="message-container">
                                                ${conversation.response}
                                            </div>` : html``}`

                            })
                    }
                    <div class="message-container">
                        ${this.completionResult_}
                    </div>
                </div>
                <div class="action-buttons-container">
                    ${this.siteInfo_.isContentUsableInConversations && this.conversations_ && this.conversations_.length == 0 ?
                            this.actionList_.map((item, _) => html`
                                <button ?disabled="${this.isSubmittingQuery_}" @click="${(e: Event) => {
                                    e.stopPropagation();
                                    this.onSubmitAction_(item.actionType);
                                }}" class="action-button">
                                    ${item.label}
                                </button>
                            `) : html``}
                </div>
            </div>
            <div id="prompt-container">
                ${
                        this.siteInfo_.isContentUsableInConversations ?
                                html`
                                    <div class="siteinfo-container">
                                        <div class="close-btn"></div>
                                        <div class="vertical-bar"></div>
                                        <div class="siteinfo-content">
                                            <div class="siteinfo-title"> ${this.siteInfo_.title}</div>
                                            <div class="siteinfo-url">
                                                ${this.stripUrlProtocol(this.siteInfo_.url ?? "")}
                                            </div>
                                        </div>
                                    </div>
                                ` : html``
                }
                <div class="typing-content">
                    <cr-input
                            type="text"
                            class="prompt-input"
                            placeholder="Ask anything..."
                            .value=${this.query_}
                            @value-changed=${this.onTextareaValueChanged_}>
                    </cr-input>
                    <button class="send-btn" @click="${this.onSubmitQuery_}">
                        Send
                    </button>
                </div>
            </div>
        </div>`;
}