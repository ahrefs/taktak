import {
    ActionItem,
    ActionType,
    PageCallbackRouter,
    PageHandlerFactory,
    PageHandlerRemote,
    SiteInfo
} from "./chat.mojom-webui.js";

export interface ChatApiProxy {
    getActionList(): Promise<{ actionList: ActionItem[] }>;

    submitAction(actionType: ActionType): void;

    submitQuery(actionType: ActionType, query: string): void;

    getSiteInfo(): Promise<{ siteInfo: SiteInfo }>

    showUI(): void;

    closeUI(): void;

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

    submitAction(actionType: ActionType) {
        this.handler.submitAction(actionType);
    }

    submitQuery(actionType: ActionType, query: string) {
        this.handler.submitQuery(actionType, query);
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

    getCallbackRouter() {
        return this.callbackRouter;
    }
}