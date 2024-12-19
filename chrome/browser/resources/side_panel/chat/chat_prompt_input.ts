import type {PropertyValues} from '//resources/lit/v3_0/lit.rollup.js';
import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';
import {getCss} from './chat_prompt_input.css.js';
import {getHtml} from './chat_prompt_input.html.js';

export interface ChatPromptInputElement {
    $: {
        input: HTMLTextAreaElement,
        label: HTMLElement,
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

            maxlength: {type: Number},

            readonly: {
                type: Boolean,
                reflect: true,
            },
            rows: {
                type: Number,
                reflect: true,
            },
            label: {type: String},

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

    override autofocus: boolean = false;
    disabled: boolean = false;
    readonly: boolean = false;
    required: boolean = false;
    rows: number = 3;
    label: string = '';
    maxlength?: number;
    value: string = '';
    placeholder: string = '';
    protected internalValue_: string = '';

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

    /**
     * 'change' event fires when <input> value changes and user presses 'Enter'.
     * This function helps propagate it to host since change events don't
     * propagate across Shadow DOM boundary by default.
     */
    protected async onInputChange_(e: Event) {
        // Ensure that |value| has been updated before re-firing 'change'.
        await this.updateComplete;
        this.dispatchEvent(new CustomEvent(
            'change', {bubbles: true, composed: true, detail: {sourceEvent: e}}));
    }


    protected onKeydown_(e: KeyboardEvent) {
        if (e.key === 'Enter') {
            if (e.shiftKey) {
                return; // Allow the default behavior
            } else {
                e.stopPropagation();
                e.preventDefault(); // Prevent adding a new line
                this.fire('enter', {value: this.value});
            }
        }
    }

    protected onInput_(e: Event) {
        this.internalValue_ = (e.target as HTMLInputElement).value;
        this.value = this.internalValue_;

        //    const lineHeight = parseInt(window.getComputedStyle(this.$.input).lineHeight);
    }

    protected onInputFocusChange_() {
        // focused_ is used instead of :focus-within, so focus on elements within
        // the suffix slot does not trigger a change in input styles.
        if (this.shadowRoot!.activeElement === this.$.input) {
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
