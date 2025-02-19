import 'chrome://resources/cr_elements/cr_icon/cr_icon.js';
import 'chrome://resources/cr_elements/icons_lit.html.js';
import {getTrustedHTML} from 'chrome://resources/js/parse_html_subset.js';
import {ChatAppElement, ActionOnExtractedContent} from "./chat_app.js";
import {html} from '//resources/lit/v3_0/lit.rollup.js';
import {marked} from "./marked.js";
import {ActionType} from "./chat.mojom-webui.js";
import type {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';
import './chat_prompt_input.js';
import './action_menu.js';

function getSiteInfoOrAddChatAboutThisPage(this: ChatAppElement) {
    if (this.shouldDisplayChatAboutThisPageButton_ && this.siteInfo_.isContentUsableInConversations) {
        return html`
            <button class="chat-about-this-page-btn"
                    @click="${(e: Event) => {
                        e.preventDefault();
                        this.onPerformActionOnExtractedContent_(ActionOnExtractedContent.AddAsContext);
                    }}">
                <div class="add-icon-wrapper">
                    <cr-icon aria-hidden="true" icon="cr:add" class="add-icon"></cr-icon>
                </div>
                <div class="label">${this.chatAboutThisPageLabel_}</div>
            </button>`;
    } else if (this.shouldHideContextActionElementsInPromptInputDueToKnownContext_) {
        return html``;
    } else if (!this.shouldDisplayChatAboutThisPageButton_ && this.siteInfo_.isContentUsableInConversations) {
        return html`
            <div class="siteinfo-container">
                <button class="remove-siteinfo-button"
                        @click="${(e: Event) => {
                            e.preventDefault();
                            this.onPerformActionOnExtractedContent_(ActionOnExtractedContent.RemoveFromUsingAsContext);
                        }}">
                    <cr-icon aria-hidden="true" icon="cr:close" class="remove-icon"></cr-icon>
                </button>
                <div class="vertical-bar"></div>
                <div class="siteinfo-content">
                    <div class="siteinfo-title"> ${this.siteInfo_.title}</div>
                    <div class="siteinfo-url">
                        ${this.stripUrlProtocol_(this.siteInfo_.url ?? "")}
                    </div>
                </div>
            </div>`
    } else {
        return html``;
    }
}

function getThinkingElement(this: ChatAppElement, thinkingText: string, id: string, isThinking: boolean, showThinkingText: boolean) {
    if (thinkingText.trim().length == 0) {
        return html``;
    }
    return html`
        <div class="thinking-button-and-text-container">
            <button class="thinking-button" @click="${(e: Event) => {
                e.preventDefault();
                const _id = id;
                this.onThinkingButtonClick_(_id);
            }}">
                <cr-icon aria-hidden="true" class="expand-less-more-icon" icon="${showThinkingText ? 'cr:expand-less' : 'cr:expand-more'}"></cr-icon>
                <span>${isThinking ? this.thinkingBtnLabel_ : this.doneThinkingBtnLabel_}</span>
            </button>
            ${
                    showThinkingText ? html`
                        <div class="thinking-text">
                            <div class="thinking-markdown-container"
                                 .innerHTML="${getTrustedHTML(marked.parse(thinkingText, {async: false}))}">
                            </div>
                        </div>` : html``
            }
        </div>`;
}

function getResponseElement(response: string) {
    if (response.trim().length == 0) {
        return html``;
    }
    return html`
        <div class="response-markdown-container"
             .innerHTML="${getTrustedHTML(marked.parse(response, {async: false}))}">
        </div>`;
}

function getThinkingAndResponseElements(this: ChatAppElement, thinkingText: string, responseText: string, id: string, isThinking: boolean, showThinkingText: boolean) {
    if (thinkingText.trim().length == 0 && responseText.trim().length == 0) {
        return html``;
    }

    return html`
        <div class="thinking-and-response-container">
            ${getThinkingElement.bind(this)(thinkingText, id, isThinking, showThinkingText)}
            ${getResponseElement(responseText)}
        </div>`;
}

function getQueryPromptSection(query: string) {
    return html`
        <div class="prompt-section">${query}</div>`;
}

export function getHtml(this: ChatAppElement) {
    return html`
        <div id="main-container">
            <div id="header-container">
                <div class="header-title-container">
                    <button id="target-close-btn" class="header-btn" @click="${this.onCloseSidePanel_}">
                        <cr-icon aria-hidden="true" icon="cr:close" class="header-icon"></cr-icon>
                    </button>
                    <div class="chat-title">${this.title_}</div>
                </div>
                <button id="target-restart-btn" class="header-btn"
                        ?disabled="${this.isSubmittingQuery_}"
                        @click="${this.onRestartChat_}">
                    <cr-icon aria-hidden="true" icon="cr:delete" class="header-icon"></cr-icon>
                </button>
            </div>
            <div id="conversation-container" class="chat-scroller chat-scroller-top-of-page">
                <div class="conversation-content">
                    <div class="gap-between-header-and-first-conversation"></div>
                    ${
                            this.conversations_.map(conversation => {
                                // if there is no user prompt or query to display due to some errors, we will only display LLM's response and thinking if any
                                if (conversation.query.length == 0) {
                                    return html`${getThinkingAndResponseElements.bind(this)(conversation.thinkingText, conversation.responseText,
                                            conversation.id, false, conversation.showThinkingText)}`;
                                }

                                if (conversation.shouldDisplaySiteInfo) {
                                    return html`
                                        <div class="query-prompt-container content-wide-width">
                                            <div class="siteinfo-container siteinfo-button"
                                                 @click="${(e: MouseEvent | KeyboardEvent) => {
                                                     // capture URL here so that the correct url will be open 
                                                     // even if multiple browser windows are open
                                                     const url = "https://" + conversation.url;
                                                     const modifier: ClickModifiers = {
                                                         middleButton: false,
                                                         altKey: e.altKey,
                                                         ctrlKey: e.ctrlKey,
                                                         metaKey: e.metaKey,
                                                         shiftKey: e.shiftKey,
                                                     };
                                                     this.openUrl_(url, modifier);
                                                 }}"
                                                 @auxclick="${(e: MouseEvent) => {
                                                     if (e.button !== 1) {
                                                         // not a middle click
                                                         return;
                                                     }
                                                     // capture URL here so that the correct url will be open 
                                                     // even if multiple browser windows are open
                                                     const url = "https://" + conversation.url;
                                                     const modifier: ClickModifiers = {
                                                         middleButton: true,
                                                         altKey: e.altKey,
                                                         ctrlKey: e.ctrlKey,
                                                         metaKey: e.metaKey,
                                                         shiftKey: e.shiftKey,
                                                     };
                                                     this.openUrl_(url, modifier);
                                                 }}">
                                                <div class="vertical-bar"></div>
                                                <div class="siteinfo-content">
                                                    <div class="siteinfo-title">${conversation.title}</div>
                                                    <div class="siteinfo-url">${conversation.url}</div>
                                                </div>
                                            </div>
                                            ${getQueryPromptSection(conversation.query)}
                                        </div>
                                        ${getThinkingAndResponseElements.bind(this)(conversation.thinkingText, conversation.responseText,
                                                conversation.id, false, conversation.showThinkingText)}`;
                                } else {
                                    return html`
                                        <div class="query-prompt-container content-fit-width">
                                            ${getQueryPromptSection(conversation.query)}
                                        </div>
                                        ${getThinkingAndResponseElements.bind(this)(conversation.thinkingText, conversation.responseText,
                                                conversation.id, false, conversation.showThinkingText)}`;
                                }
                            })
                    }
                    ${getThinkingAndResponseElements.bind(this)(this.currentThinkingResult_, this.currentResponseResult_,
                            this.currentConversationId_, this.isThinking_, this.showThinkingText_)}
                </div>
                <div class="action-buttons-container">
                    ${this.shouldShowActionsMenu_ && !this.isSubmittingQuery_ ?
                            this.actionList_.map((item, _) => {
                                if (item.actionType == ActionType.TRANSLATE) {
                                    return html`
                                        <action-menu
                                                .actionType_="${item.actionType}"
                                                .actionLabel_="${item.label}"
                                                .actionItems_="${this.translateToLanguages_}"
                                                @item-click="${this.onActionMenuItemClick_}">
                                        </action-menu>`;
                                } else if (item.actionType == ActionType.DRAFT_SOCIAL_MEDIA_POST) {
                                    return html`
                                        <action-menu
                                                .actionType_="${item.actionType}"
                                                .actionLabel_="${item.label}"
                                                .actionItems_="${this.socialMediaPlatforms_}"
                                                @item-click="${this.onActionMenuItemClick_}">
                                        </action-menu>`;
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
            </div>
            <div id="thinking-toggle-button-container">
                <button class="thinking-toggle-button" @click="${this.onThinkingToggleButtonClick_}">
                    <span>${"Enable thinking"}</span>
                </button>
            </div>
            <div id="prompt-input-container">
                ${getSiteInfoOrAddChatAboutThisPage.bind(this)()}
                <div class="typing-content">
                    <div class="prompt-input">
                        <chat-prompt-input
                                id="promptInput"
                                .autofocus=${true}
                                .value=${this.query_ ?? ""}
                                .placeholder=${this.askAnythingLabel_ ?? ""}
                                .maxlength=${this.maxPromptInputLength_}
                                ?disabled=${this.isSubmittingQuery_}
                                @value-changed=${this.onPromptInputChange_}
                                @enter=${this.onSetAndSubmitQuery_}>
                        </chat-prompt-input>
                    </div>
                    <button class="send-btn"
                            ?disabled="${(this.query_ == "" && !this.isSubmittingQuery_) || this.hasExceededMaxTokenCount_}"
                            @click="${this.isSubmittingQuery_ ? this.onCancelQuery_ : this.onSubmitQuery_}">
                        <div class="send-icon-container">
                            <cr-iconset name="query">
                                <svg>
                                    <g id="send">
                                        <path d="M9 14H11V9.8L12.6 11.4L14 10L10 6L6 10L7.4 11.4L9 9.8V14ZM10 20C8.61667 20 7.31667 19.7375 6.1 19.2125C4.88333 18.6875 3.825 17.975 2.925 17.075C2.025 16.175 1.3125 15.1167 0.7875 13.9C0.2625 12.6833 0 11.3833 0 10C0 8.61667 0.2625 7.31667 0.7875 6.1C1.3125 4.88333 2.025 3.825 2.925 2.925C3.825 2.025 4.88333 1.3125 6.1 0.7875C7.31667 0.2625 8.61667 0 10 0C11.3833 0 12.6833 0.2625 13.9 0.7875C15.1167 1.3125 16.175 2.025 17.075 2.925C17.975 3.825 18.6875 4.88333 19.2125 6.1C19.7375 7.31667 20 8.61667 20 10C20 11.3833 19.7375 12.6833 19.2125 13.9C18.6875 15.1167 17.975 16.175 17.075 17.075C16.175 17.975 15.1167 18.6875 13.9 19.2125C12.6833 19.7375 11.3833 20 10 20Z"></path>
                                    </g>
                                    <g id="stop">
                                        <path d="M6 14H14V6H6V14ZM10 20C8.61667 20 7.31667 19.7375 6.1 19.2125C4.88333 18.6875 3.825 17.975 2.925 17.075C2.025 16.175 1.3125 15.1167 0.7875 13.9C0.2625 12.6833 0 11.3833 0 10C0 8.61667 0.2625 7.31667 0.7875 6.1C1.3125 4.88333 2.025 3.825 2.925 2.925C3.825 2.025 4.88333 1.3125 6.1 0.7875C7.31667 0.2625 8.61667 0 10 0C11.3833 0 12.6833 0.2625 13.9 0.7875C15.1167 1.3125 16.175 2.025 17.075 2.925C17.975 3.825 18.6875 4.88333 19.2125 6.1C19.7375 7.31667 20 8.61667 20 10C20 11.3833 19.7375 12.6833 19.2125 13.9C18.6875 15.1167 17.975 16.175 17.075 17.075C16.175 17.975 15.1167 18.6875 13.9 19.2125C12.6833 19.7375 11.3833 20 10 20Z"></path>
                                    </g>
                                </svg>
                            </cr-iconset>
                            <cr-icon icon="${this.isSubmittingQuery_ ? 'query:stop' : 'query:send'}"
                                     class="send-icon"></cr-icon>
                        </div>
                    </button>
                </div>
            </div>
            ${this.hasExceededMaxTokenCount_
                    ? html`
                        <div id="error-container">${this.exceedMaxTokenCountErrorMessages_}</div>`
                    : html``}
        </div>`;
}