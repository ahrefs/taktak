// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {html, nothing} from '//resources/lit/v3_0/lit.rollup.js';
import type {ChatPromptInputElement} from './chat_prompt_input.js';

export function getHtml(this: ChatPromptInputElement) {
    return html`
        <textarea id="input"
                  ?autofocus="${this.autofocus}"
                  .rows="${this.rows}"
                  .value="${this.internalValue_}"
                  aria-label="chat-prompt-input"
                  @input="${this.onInput_}"
                  @focus="${this.onInputFocusChange_}"
                  @blur="${this.onInputFocusChange_}"
                  @change="${this.onInputChange_}"
                  @keydown="${this.onKeydown_}"
                  ?disabled="${this.disabled}"
                  ?readonly="${this.readonly}"
                  ?required="${this.required}"
                  placeholder="${this.placeholder || nothing}">
            </textarea>`;
}
