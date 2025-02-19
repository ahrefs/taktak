import {
    ActionItem,
    ActionType,
    PageCallbackRouter,
    PageHandlerFactory,
    PageHandlerRemote,
    SiteInfo,
    ConversationItem,
} from "./chat.mojom-webui.js";

import type {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';

export interface ChatApiProxy {
    getActionList(): Promise<{ actionList: ActionItem[] }>;

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