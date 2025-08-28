// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type {PropertyValues} from '//resources/lit/v3_0/lit.rollup.js';
import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';
import {getCss} from './chat_prompt_input.css.js';
import {getHtml} from './chat_prompt_input.html.js';

export interface ChatPromptInputElement {
    $: {
        input: HTMLTextAreaElement,
    };
}

export class ChatPromptInputElement extends CrLitElement {
    static get is() {
        return 'chat-prompt-input';
    }

    static override get styles() {
        return getCss();
    }

    override render() {
        return getHtml.bind(this)();
    }

    static override get properties() {
        return {
            autofocus: {
                type: Boolean,
                reflect: true,
            },

            disabled: {
                type: Boolean,
                reflect: true,
            },

            required: {
                type: Boolean,
                reflect: true,
            },

            maxlength: {
                type: Number,
                reflect: true,
            },

            readonly: {
                type: Boolean,
                reflect: true,
            },

            rows: {
                type: Number,
                reflect: true,
            },

            /**
             * Text inside the text area. If the text exceeds the bounds of the text
             * area, i.e. if it has more than |rows| lines, a scrollbar is shown by
             * default when autogrow is not set.
             */
            value: {
                type: String,
                notify: true,
            },

            internalValue_: {
                type: String,
                state: true,
            },

            placeholder: {type: String},
        };
    }

    override accessor autofocus: boolean = false;
    accessor disabled: boolean = false;
    protected accessor required: boolean = false;
    protected accessor maxlength!: number;
    protected accessor readonly: boolean = false;
    protected accessor rows: number = 1;
    accessor value: string = '';
    protected accessor internalValue_: string = '';
    protected accessor placeholder: string = '';

    override willUpdate(changedProperties: PropertyValues<this>) {
        super.willUpdate(changedProperties);

        if (changedProperties.has('value')) {
            // Don't allow null or undefined as these will render in the input.
            // cr-textarea cannot use Lit's "nothing" in the HTML template; this
            // breaks the underlying native textarea's auto validation if |required|
            // is set.
            this.internalValue_ =
                (this.value === undefined || this.value === null) ? '' : this.value;
        }
    }

    override updated(changedProperties: PropertyValues<this>) {
        super.updated(changedProperties);
        if (changedProperties.has('disabled')) {
            this.setAttribute('aria-disabled', this.disabled ? 'true' : 'false');
        }
    }

    focusInput() {
        this.$.input.focus();
    }

    resetToAutoHeight() {
        this.$.input.style.height = 'auto';
    }

    protected async onInputChange_(e: Event) {
        // Ensure that |value| has been updated before re-firing 'change'.
        await this.updateComplete;
        this.dispatchEvent(new CustomEvent(
            'change', {bubbles: true, composed: true, detail: {sourceEvent: e}}));
    }

    protected onKeydown_(e: KeyboardEvent) {
        if (e.key === 'Enter') {
            const textarea = this.$.input;
            if (e.shiftKey) {
                textarea.style.paddingTop = '14px';
                return; // Allow the default behavior
            } else {
                e.stopPropagation();
                e.preventDefault(); // Prevent adding a new line
                const maxLength = this.maxlength === undefined ? Infinity : this.maxlength;
                if (this.value.length <= maxLength) {
                    this.resetToAutoHeight();
                    textarea.style.paddingTop = '0';
                    this.fire('enter', {value: this.value});
                }
            }
        }
    }

    // Display a vertical scrollbar when the content exceeds 10 lines
    protected onInput_(e: Event) {
        this.internalValue_ = (e.target as HTMLInputElement).value;
        this.value = this.internalValue_;

        const textarea = this.$.input;

        const maxLines = 9;
        textarea.style.height = '0';

        const lineHeight = parseInt(window.getComputedStyle(textarea).lineHeight);
        const paddingOffset = parseInt(window.getComputedStyle(textarea).paddingTop) +
            parseInt(window.getComputedStyle(textarea).paddingBottom);
        const contentHeight = textarea.scrollHeight - (lineHeight + paddingOffset);
        const lines = Math.ceil(contentHeight / lineHeight);

        if (lines <= maxLines) {
            textarea.style.height = `${(lines + 1) * lineHeight}px`;
            textarea.style.overflowY = 'hidden'; // No scrollbar for less than 10 lines
        } else {
            textarea.style.height = `${lineHeight * maxLines}px`;
            textarea.style.overflowY = 'scroll'; // Show scrollbar after 10 lines
        }
    }

    protected onInputFocusChange_() {
        // focused_ is used instead of :focus-within, so focus on elements within
        // the suffix slot does not trigger a change in input styles.
        if (this.shadowRoot.activeElement === this.$.input) {
            this.setAttribute('focused_', '');
        } else {
            this.removeAttribute('focused_');
        }
    }
}

declare global {
    interface HTMLElementTagNameMap {
        'chat-prompt-input': ChatPromptInputElement;
    }
}

customElements.define(ChatPromptInputElement.is, ChatPromptInputElement);
