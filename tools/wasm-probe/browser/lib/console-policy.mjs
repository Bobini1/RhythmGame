export function normalizeConsoleTextForAllowlist(text) {
    if (typeof text !== "string") {
        throw new TypeError("console text must be a string");
    }
    return text.replace(/(?:\r\n|\n)$/, "");
}

export function findAllowedConsoleRecordIndex(allowedRecords, record) {
    if (!Array.isArray(allowedRecords)) {
        throw new TypeError("allowed console records must be an array");
    }
    if (
        record === null
        || typeof record !== "object"
        || typeof record.text !== "string"
        || typeof record.type !== "string"
    ) {
        throw new TypeError("console record is invalid");
    }
    const normalizedText = normalizeConsoleTextForAllowlist(record.text);
    return allowedRecords.findIndex((allowed) => (
        allowed.text === normalizedText
        && allowed.type === record.type
    ));
}
