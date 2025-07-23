// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
    ActionItem,
    ActionType,
    PageCallbackRouter,
    PageHandlerFactory,
    PageHandlerRemote,
    SiteInfo,
    ConversationItem,
    SavableConversationModel,
    ChatState,
} from "./chat.mojom-webui.js";

import type {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';

export interface ChatApiProxy {
    getActionList(): Promise<{ actionList: ActionItem[] }>;

    getChatState(): Promise<{ chatState: ChatState }>;

    getSiteInfoFromCache(): Promise<{ siteInfo: SiteInfo }>;

    saveSiteInfo(siteInfo: SiteInfo): void;

    saveConversation(conversation: SavableConversationModel): void;

    saveThinkingState(thinkingState : boolean) : void;

    getThinkingState(): Promise<{ thinkingState: boolean }>;

    clearChatState(): void;

    submitAction(actionType: ActionType, actionParam: string, enableThinking: boolean): void;

    submitQuery(actionType: ActionType, query: string, url: string, conversation_history : ConversationItem[], enableThinking: boolean): void;

    getSiteInfo(): Promise<{ siteInfo: SiteInfo }>

    showUI(): void;

    closeUI(): void;

    cancelQuery(): void;

    openUrl(url: string, clickModifiers: ClickModifiers): void;

    getCallbackRouter(): PageCallbackRouter;
}

let instance: ChatApiProxy | null = null;

export class ChatApiProxyImpl implements ChatApiProxy {
    private readonly callbackRouter: PageCallbackRouter = new PageCallbackRouter();
    private handler: PageHandlerRemote = new PageHandlerRemote();

    constructor() {
        this.callbackRouter = new PageCallbackRouter();
        this.handler = new PageHandlerRemote();
        const factory = PageHandlerFactory.getRemote();
        factory.createPageHandler(
            this.callbackRouter.$.bindNewPipeAndPassRemote(),
            this.handler.$.bindNewPipeAndPassReceiver());
    }

    static getInstance(): ChatApiProxy {
        return instance || (instance = new ChatApiProxyImpl());
    }

    static setInstance(proxy: ChatApiProxy) {
        instance = proxy;
    }

    getActionList(): Promise<{ actionList: ActionItem[] }> {
        return this.handler.getActionList();
    }

    getChatState(): Promise<{ chatState: ChatState }> {
        return this.handler.getChatState();
    }

    getSiteInfoFromCache(): Promise<{ siteInfo: SiteInfo }> {
        return this.handler.getSiteInfoFromCache();
    }

    saveSiteInfo(siteInfo: SiteInfo) {
        this.handler.saveSiteInfo(siteInfo);
    }

    saveConversation(conversation : SavableConversationModel) {
        this.handler.saveConversation(conversation);
    }

    saveThinkingState(thinkingState: boolean) {
        this.handler.saveThinkingState(thinkingState);
    }

    getThinkingState(): Promise<{ thinkingState: boolean }> {
        return this.handler.getThinkingState();
    }

    clearChatState() {
        this.handler.clearChatState();
    }

    submitAction(actionType: ActionType, actionParam: string, enableThinking: boolean) {
        this.handler.submitAction(actionType, actionParam, enableThinking);
    }

    submitQuery(actionType: ActionType, query: string, url: string, conversation_history: ConversationItem[], enableThinking: boolean ) {
        this.handler.submitQuery(actionType, query, url, conversation_history, enableThinking);
    }

    getSiteInfo() {
        return this.handler.getSiteInfo();
    }

    showUI() {
        this.handler.showUI();
    }

    closeUI() {
        this.handler.closeUI();
    }

    cancelQuery() {
        this.handler.cancelQuery();
    }

    openUrl(url: string, clickModifiers: ClickModifiers) {
        this.handler.openURL(url, clickModifiers);
    }

    getCallbackRouter() {
        return this.callbackRouter;
    }
}