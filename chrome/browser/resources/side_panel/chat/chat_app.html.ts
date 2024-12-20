import 'chrome://resources/cr_elements/cr_icon/cr_icon.js';
import 'chrome://resources/cr_elements/icons_lit.html.js';
import {getTrustedHTML} from 'chrome://resources/js/parse_html_subset.js';
import type {conversationRecord} from "./chat_app";
import {ChatAppElement} from "./chat_app";
import {html} from '//resources/lit/v3_0/lit.rollup.js';
import {marked} from "./marked.js";
import {ActionType} from "./chat.mojom-webui.js";
import {AnchorAlignment} from '//resources/cr_elements/cr_action_menu/cr_action_menu.js';

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

function getConversationResponseElement(conversation: conversationRecord) {
    return conversation.response.length > 0 ? html`
        <article class="message-markdown-container"
                 .innerHTML="${getTrustedHTML(conversation.response)}">
        </article>` : html``;
}

export function getHtml(this: ChatAppElement) {
    return html`
        <div id="container">
            <div id="conversation-container" class="chat-scroller chat-scroller-top-of-page">
                <div class="conversation-content">
                    ${
                            this.conversations_.map((conversation, _) => {
                                return conversation.query.length > 0
                                        ? html`
                                            ${conversation.shouldDisplaySiteInfo ? html`
                                                <article class="query-prompt-container auto-width">
                                                    <div class="siteinfo-container">
                                                        <div class="vertical-bar"></div>
                                                        <div class="siteinfo-content">
                                                            <div class="siteinfo-title">${conversation.title}</div>
                                                            <div class="siteinfo-url">${conversation.url}</div>
                                                        </div>
                                                    </div>
                                                    <div class="prompt-section">${conversation.query}</div>
                                                </article>` : html`
                                                <article class="query-prompt-container content-fit-width">
                                                    <div class="prompt-section">${conversation.query}</div>
                                                </article>`
                                            }
                                            ${getConversationResponseElement.bind(this)(conversation)}`
                                        :
                                        html`${getConversationResponseElement.bind(this)(conversation)}`
                            })
                    }
                    <div class="message-markdown-container"
                         .innerHTML="${getTrustedHTML(marked.parse(this.completionResult_, {async: false}))}">
                    </div>
                </div>
                <div class="action-buttons-container">
                    ${this.siteInfo_.isContentUsableInConversations && this.conversations_ && this.conversations_.length == 0 ?
                            this.actionList_.map((item, _) => { 
                                if (item.actionType == ActionType.TRANSLATE)  {
                                    return html`
                                    <button ?disabled="${this.isSubmittingQuery_}" @click="${(e: Event) => {
                                        e.stopPropagation();
                                        e.preventDefault();
                                        this.$.translatorMenu.showAt(e.target as HTMLElement, {
                                            anchorAlignmentX: AnchorAlignment.AFTER_END,
                                            anchorAlignmentY: AnchorAlignment.CENTER,
                                        });
                                    }}" class="action-button">
                                        ${item.label}
                                    </button>`;
                                 }  else if (item.actionType == ActionType.DRAFT_SOCIAL_MEDIA_POST) {
                                    return html`
                                    <button ?disabled="${this.isSubmittingQuery_}" @click="${(e: Event) => {
                                            e.stopPropagation();
                                            e.preventDefault();
                                            this.$.socialPostMenu.showAt(e.target as HTMLElement, {
                                                anchorAlignmentX: AnchorAlignment.AFTER_END,
                                                anchorAlignmentY: AnchorAlignment.CENTER,
                                            });
                                        }}" class="action-button">
                                        ${item.label}
                                    </button>`;
                                } else {
                                return html`
                                <button ?disabled="${this.isSubmittingQuery_}" @click="${(e: Event) => {
                                    e.stopPropagation();
                                    this.onSubmitAction_(item.actionType);
                                }}" class="action-button">
                                    ${item.label}
                                </button>`;
                            }
                            }) : html``}
                </div>
                <div>
                    <cr-action-menu id="translatorMenu">
                        <button class="dropdown-item">${"Afrikaans"}</button>
                        <button class="dropdown-item">${"Albanian"}</button>
                        <button class="dropdown-item">${"Amharic"}</button>
                        <button class="dropdown-item">${"Arabic"}</button>
                        <button class="dropdown-item">${"Armenian"}</button>
                        <button class="dropdown-item">${"Assamese"}</button>
                        <button class="dropdown-item">${"Aymara"}</button>
                    </cr-action-menu> 
                    <cr-action-menu id="socialPostMenu">
                        <button class="dropdown-item">${"X (Twitter)"}</button>
                        <button class="dropdown-item">${"Facebook"}</button>
                        <button class="dropdown-item">${"Instagram"}</button>
                        <button class="dropdown-item">${"Linkedin"}</button>
                    </cr-action-menu> 
                </div>
            </div>
            <div id="prompt-container">
                ${getSiteInfoOrAddChatAboutThisPage.bind(this)()}
                <div class="typing-content">
                    <div class="prompt-input">
                        <chat-prompt-input
                                .placeholder=${this.askAnythingLabel_ ?? ""}
                                .value=${this.query_ ?? ""}
                                .autofocus=${true}
                                ?disabled=${this.isSubmittingQuery_}
                                @value-changed=${this.onPromptInputChange_}
                                @enter=${this.onSetAndSubmitQuery_}>
                        </chat-prompt-input>
                    </div>
                    <button class="send-btn" ?disabled="${this.query_ == "" || this.isSubmittingQuery_}" @click="${this.onSubmitQuery_}">
                        <cr-icon aria-hidden="true"
                                 icon="cr:arrow-drop-up" class="send-btn-icon">
                        </cr-icon>
                    </button>
                </div>
            </div>
        </div>`;
}
