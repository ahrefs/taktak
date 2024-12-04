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
                                                        <div class="siteinfo-section">
                                                            <div class="vertical-bar"></div>
                                                            <div class="content">
                                                                <div class="title">${conversation.title}</div>
                                                                <div class="url">${conversation.url}</div>
                                                            </div>
                                                        </div>
                                                        <div class="prompt-section">${conversation.query}</div>
                                                    </div>`: html`
                                                    <div class="query-prompt-container content-fit-width">
                                                        <div class="prompt-section">${conversation.query}</div>
                                                    </div>`
                                                }
                                            ${conversation.response.length > 0 ? html`<div class="message-container">
                                                ${conversation.response}</div>` : html``}`
                                        :
                                        html`${conversation.response.length > 0 ? html`<div class="message-container">
                                            ${conversation.response}</div>` : html``}`

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
                                    <div class="site-info-container">
                                        <p class="site-info-content">${this.siteInfo_.title}</p>
                                        <p class="site-info-content">${this.stripUrlProtocol(this.siteInfo_.url ?? "")} </p>
                                    </div>` : html``
                }
                <div class="typing-content">
                    <cr-textarea
                            class="prompt-input"
                            placeholder="Ask anything..."
                            .value=${this.query_}
                            @value-changed=${this.onTextareaValueChanged_}>
                    </cr-textarea>
                    <button class="send-btn" @click="${this.onSubmitQuery_}">
                        Send
                    </button>
                </div>
            </div>
        </div>`;
}