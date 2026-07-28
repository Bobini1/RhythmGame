self.addEventListener("message", (event) => {
    const { nonce } = event.data ?? {};
    if (typeof nonce !== "string" || nonce.length === 0) {
        self.postMessage({ error: "invalid-nonce" });
        return;
    }
    self.postMessage({ nonce, transport: "dedicated-worker" });
});
