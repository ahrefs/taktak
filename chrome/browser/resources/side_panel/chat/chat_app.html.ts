// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_icon/cr_icon.js';
import 'chrome://resources/cr_elements/icons.html.js';
import 'chrome://resources/cr_elements/cr_icon/cr_iconset.js';

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
                <span>${this.chatAboutThisPageLabel_}</span>
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

function getThinkingElement(this: ChatAppElement,
                            thinkingText: string,
                            id: string,
                            isThinking: boolean,
                            showThinkingText: boolean) {
    if (thinkingText.trim().length == 0) {
        return html``;
    }

    return html`
        <div class="thinking-button-and-text-container">
            <button class="thinking-button" @click="${(e: Event) => {
                e.preventDefault();
                this.onThinkingButtonClick_(id);
            }}">
                <cr-icon aria-hidden="true" class="expand-less-more-icon"
                         icon="${showThinkingText ? 'cr:expand-less' : 'cr:expand-more'}"></cr-icon>
                <span>${isThinking ? this.thinkingBtnLabel_ : this.doneThinkingBtnLabel_}</span>
            </button>
            ${
                    showThinkingText ? html`
                        <div class="thinking-text">
                            <div class="thinking-markdown-container"
                                 .innerHTML="${getTrustedHTML(marked.parse(thinkingText, {async: false, gfm: false}))}">
                            </div>
                        </div>` : html``
            }
        </div>`;
}

function getResponseElement(responseText: string) {
    if (responseText.trim().length == 0) {
        return html``;
    }
    return html`
        <div class="response-markdown-container"
             .innerHTML="${getTrustedHTML(marked.parse(responseText, {async: false}))}">
        </div>`;
}

function getErrorElement(errorText: string) {
    if (errorText.trim().length == 0) {
        return html``;
    }
    return html`
        <div class="generic-error-container">
            <cr-icon aria-hidden="true" icon="cr:error-outline" class="generic-error-icon"></cr-icon>
            <span class="generic-error-text">${errorText}</span>
        </div>`;
}

function getThinkingAndResponseElements(this: ChatAppElement,
                                        thinkingText: string,
                                        responseText: string,
                                        id: string,
                                        isThinking: boolean,
                                        showThinkingText: boolean,
                                        errorText: string) {
    if (thinkingText.trim().length == 0 && responseText.trim().length == 0 && errorText.trim().length == 0) {
        return html``;
    }

    return html`
        <div class="thinking-and-response-container">
            ${getThinkingElement.bind(this)(thinkingText, id, isThinking, showThinkingText)}
            ${getResponseElement(responseText)}
            ${getErrorElement(errorText)}
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
                    ${
                            // If chat is open via chrome://chat, the title of the tab is null; in this case we won't show the close button on the Chat UI header
                            this.siteInfo_.title === null
                                    ? html``
                                    : html`
                                        <button id="target-close-btn" class="header-btn" @click="${this.onCloseSidePanel_}">
                                            <cr-icon aria-hidden="true" icon="cr:close" class="header-icon"></cr-icon>
                                        </button>`
                    }
                    <div class="chat-title">${this.title_}</div>
                </div>
                <button id="target-restart-btn" class="header-btn"
                        @click="${this.onDeleteAll_}">
                    <cr-icon aria-hidden="true" icon="cr:delete" class="header-icon"></cr-icon>
                </button>
            </div>
            <div id="conversationContainer" class="chat-scroller chat-scroller-top-of-page">
                <div class="conversation-content">
                    <div class="gap-between-header-and-first-conversation"></div>
                    ${
                            this.conversations_.map(conversation => {
                                // if there is no user prompt or query to display due to some errors, we will only display LLM's response and thinking if any
                                if (conversation.query.length == 0) {
                                    return html`${getThinkingAndResponseElements.bind(this)(
                                            conversation.thinkingText,
                                            conversation.responseText,
                                            conversation.id,
                                            false,
                                            conversation.showThinkingText,
                                            conversation.errorText)}`;
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
                                        ${getThinkingAndResponseElements.bind(this)(
                                                conversation.thinkingText,
                                                conversation.responseText,
                                                conversation.id,
                                                false,
                                                conversation.showThinkingText,
                                                conversation.errorText
                                        )}`;
                                } else {
                                    return html`
                                        <div class="query-prompt-container content-fit-width">
                                            ${getQueryPromptSection(conversation.query)}
                                        </div>
                                        ${getThinkingAndResponseElements.bind(this)(
                                                conversation.thinkingText,
                                                conversation.responseText,
                                                conversation.id,
                                                false,
                                                conversation.showThinkingText,
                                                conversation.errorText
                                        )}`;
                                }
                            })
                    }
                    ${getThinkingAndResponseElements.bind(this)(
                            this.currentThinkingResult_,
                            this.currentResponseResult_,
                            this.currentConversationId_,
                            this.isThinking_,
                            this.showThinkingText_,
                            this.currentErrorResult_
                    )}
                </div>
                <div class="action-buttons-container">
                    ${this.shouldShowActionsMenu_ && !this.isQuerySubmitting_ ?
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
                                        <button ?disabled="${this.isQuerySubmitting_}" @click="${(e: Event) => {
                                            e.stopPropagation();
                                            this.onSubmitAction_(item.actionType);
                                        }}" class="action-button">
                                            ${item.label}
                                        </button>`;
                                }
                            }) : html``}
                </div>
            </div>
            ${
                    this.conversations_.length == 0 ? html`
                        <div id="thinking-toggle-button-container">
                            <button class="thinking-toggle-button" @click="${this.onToggleEnableThinking}"
                                    style="${this.enableThinking_ ? 'color: var(--color-yep-chat-thinking-enabled)' : 'color: var(--color-yep-chat-secondary-text);'}">
                                <div>
                                    <cr-iconset name="thinking-icon-set">
                                        <svg>
                                            <g id="head">
                                                <path
                                                        d="M10.275 13.526C11.4417 13.526 12.4333 13.151 13.25 12.401C14.0667 11.651 14.475 10.7426 14.475 9.67598C14.475 8.72598 14.1708 7.92181 13.5625 7.26348C12.9542 6.60514 12.2167 6.27598 11.35 6.27598C10.5667 6.27598 9.90417 6.52598 9.3625 7.02598C8.82083 7.52598 8.55 8.14264 8.55 8.87598C8.55 9.19264 8.6125 9.50098 8.7375 9.80098C8.8625 10.101 9.04167 10.376 9.275 10.626L10.7 9.20098C10.65 9.16764 10.6125 9.12598 10.5875 9.07598C10.5625 9.02598 10.55 8.96764 10.55 8.90098C10.55 8.71764 10.625 8.57181 10.775 8.46348C10.925 8.35514 11.1167 8.30098 11.35 8.30098C11.6833 8.30098 11.9583 8.43848 12.175 8.71348C12.3917 8.98848 12.5 9.31764 12.5 9.70098C12.5 10.2176 12.2875 10.6551 11.8625 11.0135C11.4375 11.3718 10.9167 11.551 10.3 11.551C9.51667 11.551 8.85417 11.2343 8.3125 10.601C7.77083 9.96764 7.5 9.19264 7.5 8.27598C7.5 7.79264 7.59167 7.33014 7.775 6.88848C7.95833 6.44681 8.21667 6.05931 8.55 5.72598L7.125 4.30098C6.59167 4.81764 6.18333 5.41764 5.9 6.10098C5.61667 6.78431 5.475 7.50098 5.475 8.25098C5.475 9.71764 5.94167 10.9635 6.875 11.9885C7.80833 13.0135 8.94167 13.526 10.275 13.526ZM4 20.001V15.701C3.05 14.8343 2.3125 13.8218 1.7875 12.6635C1.2625 11.5051 1 10.2843 1 9.00098C1 6.50098 1.875 4.37598 3.625 2.62598C5.375 0.875977 7.5 0.000976562 10 0.000976562C12.0833 0.000976562 13.9292 0.613477 15.5375 1.83848C17.1458 3.06348 18.1917 4.65931 18.675 6.62598L19.975 11.751C20.0583 12.0676 20 12.3551 19.8 12.6135C19.6 12.8718 19.3333 13.001 19 13.001H17V16.001C17 16.551 16.8042 17.0218 16.4125 17.4135C16.0208 17.8051 15.55 18.001 15 18.001H13V20.001H11V16.001H15V11.001H17.7L16.75 7.12598C16.3667 5.60931 15.55 4.37598 14.3 3.42598C13.05 2.47598 11.6167 2.00098 10 2.00098C8.06667 2.00098 6.41667 2.67598 5.05 4.02598C3.68333 5.37598 3 7.01764 3 8.95098C3 9.95098 3.20417 10.901 3.6125 11.801C4.02083 12.701 4.6 13.501 5.35 14.201L6 14.801V20.001H4Z"/>
                                            </g>
                                        </svg>
                                    </cr-iconset>
                                    <cr-icon icon="${'thinking-icon-set:head'}"
                                             class="thinking-toggle-button-head-icon"
                                             style="${this.enableThinking_ ? 'color: var(--color-yep-chat-thinking-enabled); opacity: 0.5;' : 'color: var(--color-yep-chat-tertiary-text); opacity: 1.0;'}"
                                    ></cr-icon>
                                </div>
                                <span class="thinking-toggle-button-label">${this.enableThinking_ ? this.thinkingEnabledBtnLabel_ : this.enableThinkingBtnLabel_}</span>
                                ${
                                        this.enableThinking_ ? html`
                                            <cr-icon icon="cr:clear"
                                                     class="thinking-toggle-button-close-icon"></cr-icon>
                                        ` : html``
                                }
                            </button>
                        </div>` : html``
            }
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
                                ?disabled=${this.isQuerySubmitting_}
                                @value-changed=${this.onPromptInputChange_}
                                @enter=${this.onSetAndSubmitQuery_}>
                        </chat-prompt-input>
                    </div>
                    <button class="send-btn"
                            ?disabled="${(this.query_ == "" && !this.isQuerySubmitting_) || this.hasExceededMaxTokenCount_}"
                            @click="${this.isQuerySubmitting_ ? this.onCancelQuery_ : this.onSubmitQuery_}">
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
                            <cr-icon
                                    icon="${this.isQuerySubmitting_ ? 'query:stop' : 'query:send'}"
                                    class="send-icon"></cr-icon>
                        </div>
                    </button>
                </div>
            </div>
            ${this.hasExceededMaxTokenCount_
                    ? html`
                        <div id="input-error-container">${this.exceedMaxTokenCountErrorMessages_}</div>`
                    : html``}
        </div>`;
}