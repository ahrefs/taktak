// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {html} from '//resources/lit/v3_0/lit.rollup.js';

import type {ActionMenuElement} from './action_menu.js';

export function getHtml(this: ActionMenuElement) {
    return html`
        <button id="actionMenuButton" class="action-button" @click="${this.onActionMenuButtonClick_}">
            ${this.actionLabel_}
        </button>

        ${this.renderActionMenu_ ? html`
            <cr-action-menu>
                ${this.actionItems_.map((item, _) => {
                    return html`
                        <button class="dropdown-item" @click="${(event: Event) => {
                            event.preventDefault();
                            event.stopPropagation();
                            this.onActionMenuItemClick_(item);
                        }
                        }">
                            <span>${item}</span>
                        </button>`;
                })}
            </cr-action-menu>` : html``}`;
}
