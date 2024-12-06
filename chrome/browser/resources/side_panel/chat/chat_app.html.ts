import {ChatAppElement} from "./chat_app";
import {html} from '//resources/lit/v3_0/lit.rollup.js';
import 'chrome://resources/cr_elements/cr_icon/cr_icon.js';
import 'chrome://resources/cr_elements/icons_lit.html.js';

function getSiteInfoOrAddChatAboutThisPage(this: ChatAppElement) {
    if (this.shouldDisplayChatAboutThisPageButton_ && this.siteInfo_.isContentUsableInConversations) {
        return html`
            <div class="chat-about-this-page-container">
                <button class="add-button" @click="${() => this.shouldDisplayChatAboutThisPageButton(false)}">
                    <cr-icon aria-hidden="true" icon="cr:add" class="add-icon"></cr-icon>
                </button>
                <div class="label">${this.chatAboutThisPageLabel_}</div>
            </div>`;
    } else if (!this.shouldDisplayChatAboutThisPageButton_ && this.siteInfo_.isContentUsableInConversations) {
        return html`
            <div class="siteinfo-container">
                <button class="remove-siteinfo-button"
                        @click="${() => this.shouldDisplayChatAboutThisPageButton(true)}">
                    <cr-icon aria-hidden="true" icon="cr:close" class="remove-icon"></cr-icon>
                </button>
                <div class="vertical-bar"></div>
                <div class="siteinfo-content">
                    <div class="siteinfo-title"> ${this.siteInfo_.title}</div>
                    <div class="siteinfo-url">
                        ${this.stripUrlProtocol(this.siteInfo_.url ?? "")}
                    </div>
                </div>
            </div>`
    } else {
        return html``;
    }
}

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
                ${getSiteInfoOrAddChatAboutThisPage.bind(this)()}
                <div class="typing-content">
                    <textarea class="prompt-input"
                           placeholder=${this.askAnythingLabel_ ?? ""}
                           .value=${this.query_ ?? ""}
                           @change=${this.onPromptInputChange_}>
                    </textarea>
                    <button class="send-btn" @click="${this.onSubmitQuery_}">
                        <cr-icon aria-hidden="true"
                                 icon="cr:arrow-drop-up" class="send-btn-icon">
                        </cr-icon>
                    </button>
                </div>
            </div>
        </div>`;
}
