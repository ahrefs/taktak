// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/cr_action_menu/cr_action_menu.js';
import '//resources/cr_elements/cr_dialog/cr_dialog.js';
import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js';
import {assert} from '//resources/js/assert.js';
import {getCss} from './action_menu.css.js';
import {getHtml} from './action_menu.html.js';
import {ActionType} from "./chat.mojom-webui.js";
import {AnchorAlignment} from '//resources/cr_elements/cr_action_menu/cr_action_menu.js';

export interface ActionMenuElement {
    $: {
        actionMenuButton: HTMLElement,
    };
}

export class ActionMenuElement extends CrLitElement {

    constructor() {
        super();
    }

    static get is() {
        return 'action-menu';
    }

    static override get styles() {
        return getCss();
    }

    override render() {
        return getHtml.bind(this)();
    }

    static override get properties() {
        return {
            actionType_: {type: ActionType},
            actionLabel_: {type: String},
            actionItems_: {type: Array},
            renderActionMenu_: {type: Boolean},
            disabled_: {type: Boolean},
        };
    }

    protected accessor actionType_: ActionType = ActionType.NONE;
    protected accessor actionLabel_: string = '';
    protected accessor actionItems_: string[] = [];
    protected accessor renderActionMenu_: boolean = false;
    protected accessor disabled_: boolean = false;

    protected async onActionMenuButtonClick_(event: Event) {
        event.preventDefault();  // Prevent default browser action (navigation).
        if (!this.renderActionMenu_) {
            this.renderActionMenu_ = true;
            await this.updateComplete;
        }
        const menu = this.shadowRoot.querySelector('cr-action-menu');
        assert(menu);
        menu.showAt(this.$.actionMenuButton, {
                anchorAlignmentX: AnchorAlignment.AFTER_END,
                anchorAlignmentY: AnchorAlignment.CENTER,
            }
        );
    }

    protected onActionMenuItemClick_(action_param: string) {
        this.fire("item-click", {actionType: this.actionType_, actionParam: action_param})
    }
}


declare global {
    interface HTMLElementTagNameMap {
        'action-menu': ActionMenuElement;
    }
}

customElements.define(ActionMenuElement.is, ActionMenuElement);
