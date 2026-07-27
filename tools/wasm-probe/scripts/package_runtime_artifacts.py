from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import sys
from pathlib import Path


GENERATED_NAMES = {
    "RhythmGameWasmProbe.aw.js",
    "RhythmGameWasmProbe.js",
    "RhythmGameWasmProbe.wasm",
    "RhythmGameWasmProbe.ww.js",
}
OUTPUT_PATTERNS = (
    re.compile(r"^RhythmGameWasmProbe\.[0-9a-f]{64}\.(?:js|wasm)$"),
    re.compile(r"^RhythmGameWasmProbe\.(?:aw|ww)\.[0-9a-f]{64}\.js$"),
    re.compile(r"^(?:bootstrap|preflight-worker)\.[0-9a-f]{64}\.mjs$"),
    re.compile(r"^(?:probe)\.[0-9a-f]{64}\.(?:css|webm)$"),
    re.compile(r"^qtloader\.[0-9a-f]{64}\.js$"),
)
FIXED_OUTPUTS = {
    "RhythmGameWasmProbe.html",
    "runtime-artifacts.json",
}
MIME = {
    "audioWorklet": "text/javascript; charset=utf-8",
    "bootstrap": "text/javascript; charset=utf-8",
    "css": "text/css; charset=utf-8",
    "html": "text/html; charset=utf-8",
    "mainJs": "text/javascript; charset=utf-8",
    "media": "video/webm",
    "preflightWorker": "text/javascript; charset=utf-8",
    "qtloader": "text/javascript; charset=utf-8",
    "wasm": "application/wasm",
    "wasmWorker": "text/javascript; charset=utf-8",
}
ARTIFACT_NAMES = {
    "audioWorklet": ("RhythmGameWasmProbe.aw", "js"),
    "bootstrap": ("bootstrap", "mjs"),
    "css": ("probe", "css"),
    "mainJs": ("RhythmGameWasmProbe", "js"),
    "media": ("probe", "webm"),
    "preflightWorker": ("preflight-worker", "mjs"),
    "qtloader": ("qtloader", "js"),
    "wasm": ("RhythmGameWasmProbe", "wasm"),
    "wasmWorker": ("RhythmGameWasmProbe.ww", "js"),
}
TEXT_ROLES = {
    "audioWorklet",
    "bootstrap",
    "css",
    "html",
    "mainJs",
    "preflightWorker",
    "qtloader",
    "wasmWorker",
}
DIGEST_PATTERN = re.compile(r"^[0-9a-f]{64}$")
SRI_PATTERN = re.compile(r"^sha256-[A-Za-z0-9+/]{43}=$")
WINDOWS_BUILD_PATH_PATTERN = re.compile(
    r"(?<![A-Za-z0-9_])[A-Za-z]:[\\/][^\s\"'`<>]*"
)
UNC_PATH_PATTERN = re.compile(
    r"(?<!\\)\\{2,}(?="
    r"[^\\/:*?\"<>|\r\n]+"
    r"\\{1,2}"
    r"[^\\/:*?\"<>|\r\n]+"
    r")"
)
WINDOWS_DEVICE_PATH_PATTERN = re.compile(
    r"(?<!\\)\\{2,}[?.]\\{1,2}"
)
FILE_SCHEME_PATTERN = re.compile(
    r"(?<![A-Za-z0-9+.-])file:(?=\S)",
    re.IGNORECASE,
)
QUOTED_SLASH_TOKEN_PATTERN = re.compile(
    r"(?<![A-Za-z0-9_$@%+~.\\:/<-])"
    r"(?!/+\$\{)"
    r"/+"
    r"(?:"
    r"[A-Za-z0-9_$@%+~.-]+"
    r"(?:/[A-Za-z0-9_$@%+~.-]+)*"
    r"/?"
    r")?"
)
ABSOLUTE_MULTI_COMPONENT_PATH_PATTERN = re.compile(
    r"(?<![A-Za-z0-9_.:/-])/"
    r"(?:[A-Za-z0-9_$@%+~.-]+/)+"
    r"[A-Za-z0-9_$@%+~.-]+"
)
COMMENT_PATTERN = re.compile(
    r"(?<!\\)/\*.*?\*/|(?<![\\:])//[^\r\n]*",
    re.DOTALL,
)
COMMENT_QUOTED_PATH_PATTERN = re.compile(
    r"(?P<quote>[\"'])"
    r"(?P<value>/[^\"'\r\n]*|file://[^\"'\r\n]*)"
    r"(?P=quote)"
)
CSS_UNQUOTED_URL_PATTERN = re.compile(
    r"\burl\(\s*(/[^)\s\"']+)\s*\)",
    re.IGNORECASE,
)
DYNAMIC_CODE_EXECUTION_PATTERN = re.compile(
    r"(?<![A-Za-z0-9_$])"
    r"(?P<constructor>eval|Function)\s*\("
)
OWNED_WEBASSEMBLY_FUNCTION_PREFIX = re.compile(
    r"(?:^|[^A-Za-z0-9_$])WebAssembly\s*\.\s*$"
)
FAVICON_LINK = '<link rel="icon" href="data:,">'
ALLOWED_QUOTED_SLASH_LITERALS = frozenset({
    "/",
    "//",
    "/dev",
    "/dev/null",
    "/dev/shm",
    "/dev/shm/tmp",
    "/dev/stderr",
    "/dev/stdin",
    "/dev/stdout",
    "/dev/tty",
    "/dev/tty1",
    "/fixtures/probe.webm",
    "/home",
    "/home/web_user",
    "/path/to/destination",
    "/probe/ws",
    "/proc",
    "/proc/self",
    "/proc/self/fd",
    "/tmp",
})
BUILD_TIMESTAMP_PATTERNS = (
    re.compile(r"\b\d{4}-\d{2}-\d{2}\b"),
    re.compile(
        r"\b\d{8}T\d{6}(?:Z|[+-]\d{4})?\b"
    ),
    re.compile(
        r"\b(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)"
        r"\s{1,2}\d{1,2}\s+\d{4}\b"
    ),
    re.compile(
        r"\b(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun),?\s+"
        r"\d{1,2}\s+"
        r"(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)"
        r"\s+\d{4}(?:\s+\d{2}:\d{2}:\d{2}\s+(?:GMT|UTC))?\b"
    ),
    re.compile(r"(?<!\d)\d{2}:\d{2}:\d{2}(?!\d)"),
    re.compile(r"\b__(?:DATE|TIME)__\b"),
)


def file_bytes(path: Path, description: str) -> bytes:
    if not path.is_file():
        raise ValueError(f"missing {description}: {path}")
    return path.read_bytes()


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def sri(payload: bytes) -> str:
    encoded = base64.b64encode(hashlib.sha256(payload).digest()).decode("ascii")
    return f"sha256-{encoded}"


def content_name(stem: str, suffix: str, payload: bytes) -> str:
    return f"{stem}.{sha256(payload)}.{suffix}"


def artifact(build_id: str, mime: str, url: str, payload: bytes) -> dict:
    return {
        "buildId": build_id,
        "bytes": len(payload),
        "mime": mime,
        "sha256": sha256(payload),
        "sri": sri(payload),
        "url": url,
    }


def expected_artifact_url(role: str, digest: str) -> str:
    if role == "html":
        return "RhythmGameWasmProbe.html"
    stem, extension = ARTIFACT_NAMES[role]
    return f"{stem}.{digest}.{extension}"


def quoted_text_spans(source: str) -> list[tuple[int, int, str]]:
    spans = []
    index = 0
    while index < len(source):
        escaped = index > 0 and source[index - 1] == "\\"
        if not escaped and source.startswith("//", index):
            newline = source.find("\n", index + 2)
            index = len(source) if newline < 0 else newline + 1
            continue
        if not escaped and source.startswith("/*", index):
            closing = source.find("*/", index + 2)
            if closing < 0:
                raise ValueError("unterminated block comment")
            index = closing + 2
            continue
        if source.startswith("<!--", index):
            closing = source.find("-->", index + 4)
            if closing < 0:
                raise ValueError("unterminated HTML comment")
            index = closing + 3
            continue
        quote = source[index]
        if quote not in {"'", '"', "`"}:
            index += 1
            continue
        start = index
        content_start = index + 1
        index += 1
        while index < len(source):
            if source[index] == "\\":
                index += 2
                continue
            if source[index] == quote:
                spans.append((
                    start,
                    index + 1,
                    source[content_start:index],
                ))
                index += 1
                break
            index += 1
        else:
            raise ValueError(f"unterminated {quote} quoted text")
    return spans


def masked_quoted_text(
    source: str,
    spans: list[tuple[int, int, str]],
) -> str:
    masked = list(source)
    for start, end, _content in spans:
        masked[start:end] = " " * (end - start)
    return "".join(masked)


def normalize_direct_javascript_path_escapes(content: str) -> str:
    simple_escapes = {
        "0": "\0",
        "b": "\b",
        "f": "\f",
        "n": "\n",
        "r": "\r",
        "t": "\t",
        "v": "\v",
    }
    normalized = []
    index = 0
    while index < len(content):
        if content[index] != "\\" or index + 1 >= len(content):
            normalized.append(content[index])
            index += 1
            continue

        following = content[index + 1]
        if following == "\r":
            index += 3 if content.startswith("\r\n", index + 1) else 2
            continue
        if following in {"\n", "\u2028", "\u2029"}:
            index += 2
            continue
        if following == "x":
            digits = content[index + 2:index + 4]
            if len(digits) == 2 and all(
                value in "0123456789abcdefABCDEF"
                for value in digits
            ):
                normalized.append(chr(int(digits, 16)))
                index += 4
                continue
        if following == "u":
            if content.startswith("u{", index + 1):
                closing = content.find("}", index + 3)
                digits = (
                    content[index + 3:closing]
                    if closing >= 0
                    else ""
                )
                if (
                    1 <= len(digits) <= 6
                    and all(
                        value in "0123456789abcdefABCDEF"
                        for value in digits
                    )
                    and int(digits, 16) <= 0x10FFFF
                ):
                    normalized.append(chr(int(digits, 16)))
                    index = closing + 1
                    continue
            else:
                digits = content[index + 2:index + 6]
                if len(digits) == 4 and all(
                    value in "0123456789abcdefABCDEF"
                    for value in digits
                ):
                    normalized.append(chr(int(digits, 16)))
                    index += 6
                    continue
        if following in "01234567":
            maximum = 3 if following in "0123" else 2
            end = index + 1
            while (
                end < len(content)
                and end < index + 1 + maximum
                and content[end] in "01234567"
            ):
                end += 1
            normalized.append(chr(int(content[index + 1:end], 8)))
            index = end
            continue

        normalized.append(simple_escapes.get(following, following))
        index += 2
    return "".join(normalized)


def normalize_direct_css_path_escapes(source: str) -> str:
    normalized = []
    index = 0
    hexadecimal = "0123456789abcdefABCDEF"
    while index < len(source):
        if source[index] != "\\" or index + 1 >= len(source):
            normalized.append(source[index])
            index += 1
            continue

        following = source[index + 1]
        if following == "\r":
            index += 3 if source.startswith("\r\n", index + 1) else 2
            continue
        if following in {"\n", "\f"}:
            index += 2
            continue
        if following in hexadecimal:
            end = index + 1
            while (
                end < len(source)
                and end < index + 7
                and source[end] in hexadecimal
            ):
                end += 1
            code = int(source[index + 1:end], 16)
            normalized.append(
                chr(code)
                if 0 < code <= 0x10FFFF
                else "\uFFFD"
            )
            if source.startswith("\r\n", end):
                end += 2
            elif end < len(source) and source[end] in " \t\r\n\f":
                end += 1
            index = end
            continue

        normalized.append(following)
        index += 2
    return "".join(normalized)


def contains_private_windows_path(source: str) -> bool:
    return any(pattern.search(source) for pattern in (
        WINDOWS_BUILD_PATH_PATTERN,
        UNC_PATH_PATTERN,
        WINDOWS_DEVICE_PATH_PATTERN,
    ))


def validate_quoted_path_content(
    content: str,
    description: str,
) -> None:
    normalized = normalize_direct_javascript_path_escapes(content)
    if contains_private_windows_path(normalized):
        raise ValueError(f"{description} contains a private build path")
    if (
        FILE_SCHEME_PATTERN.search(normalized)
        and normalized != "file://"
    ):
        raise ValueError(f"{description} contains a private build path")
    for match in QUOTED_SLASH_TOKEN_PATTERN.finditer(normalized):
        if (
            (
                match.start() > 0
                and normalized[match.start() - 1] == "}"
            )
            or normalized.startswith("${", match.end())
        ):
            continue
        if match.group(0) not in ALLOWED_QUOTED_SLASH_LITERALS:
            raise ValueError(f"{description} contains a private build path")


def masked_comments(source: str) -> str:
    masked = list(source)
    for comment in COMMENT_PATTERN.finditer(source):
        masked[comment.start():comment.end()] = (
            " " * (comment.end() - comment.start())
        )
    return "".join(masked)


def contains_dynamic_code_execution(source: str) -> bool:
    for match in DYNAMIC_CODE_EXECUTION_PATTERN.finditer(source):
        if (
            match.group("constructor") == "Function"
            and OWNED_WEBASSEMBLY_FUNCTION_PREFIX.search(
                source[:match.start()]
            )
        ):
            continue
        return True
    return False


def validate_text_payload(
    payload: bytes | str,
    description: str,
    *,
    css: bool = False,
) -> str:
    source = (
        payload.decode("utf-8")
        if isinstance(payload, bytes)
        else payload
    )
    scan_source = (
        normalize_direct_css_path_escapes(source)
        if css
        else source
    )
    if contains_private_windows_path(scan_source):
        raise ValueError(f"{description} contains a private build path")
    spans = quoted_text_spans(scan_source)
    for _start, _end, content in spans:
        validate_quoted_path_content(content, description)

    masked = masked_quoted_text(scan_source, spans)
    if css and FILE_SCHEME_PATTERN.search(masked):
        raise ValueError(f"{description} contains a private build path")
    for comment in COMMENT_PATTERN.finditer(masked):
        comment_text = comment.group(0)
        if FILE_SCHEME_PATTERN.search(comment_text):
            raise ValueError(f"{description} contains a private build path")
        comment_mask = list(comment_text)
        for quoted_path in COMMENT_QUOTED_PATH_PATTERN.finditer(comment_text):
            value = quoted_path.group("value")
            validate_quoted_path_content(value, description)
            comment_mask[
                quoted_path.start():quoted_path.end()
            ] = " " * (quoted_path.end() - quoted_path.start())
        if ABSOLUTE_MULTI_COMPONENT_PATH_PATTERN.search(
            "".join(comment_mask)
        ):
            raise ValueError(f"{description} contains a private build path")
    for match in CSS_UNQUOTED_URL_PATTERN.finditer(masked):
        if match.group(1).count("/") >= 2:
            raise ValueError(f"{description} contains a private build path")
    ordinary_string_spans = [
        span
        for span in spans
        if scan_source[span[0]] in {"'", '"'}
    ]
    executable_source = masked_comments(
        masked_quoted_text(scan_source, ordinary_string_spans)
    )
    if contains_dynamic_code_execution(executable_source):
        raise ValueError(f"{description} contains dynamic code execution")
    if any(pattern.search(source) for pattern in BUILD_TIMESTAMP_PATTERNS):
        raise ValueError(f"{description} contains a build timestamp")
    return source


def validate_generated_shape(generated_dir: Path) -> None:
    if not generated_dir.is_dir():
        raise ValueError(f"missing generated directory: {generated_dir}")
    relevant = {
        entry.name
        for entry in generated_dir.iterdir()
        if entry.name.startswith("RhythmGameWasmProbe.")
    }
    missing = sorted(GENERATED_NAMES - relevant)
    if missing:
        raise ValueError(f"missing generated runtime asset: {', '.join(missing)}")
    extra = sorted(relevant - GENERATED_NAMES)
    if extra:
        raise ValueError(
            f"unexpected generated runtime asset: {', '.join(extra)}"
        )
    for name in GENERATED_NAMES:
        entry = generated_dir / name
        if entry.is_symlink() or not entry.is_file():
            raise ValueError(f"generated runtime asset is not regular: {name}")


def quoted_javascript_literals(source: str) -> list[tuple[int, int]]:
    literals = []
    index = 0
    while index < len(source):
        current = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""
        if current == "/" and following == "/":
            newline = source.find("\n", index + 2)
            index = len(source) if newline < 0 else newline + 1
            continue
        if current == "/" and following == "*":
            closing = source.find("*/", index + 2)
            if closing < 0:
                raise ValueError("generated main JavaScript has an open comment")
            index = closing + 2
            continue
        if current in {"'", '"', "`"}:
            quote = current
            content_start = index + 1
            index += 1
            while index < len(source):
                if source[index] == "\\":
                    index += 2
                    continue
                if source[index] == quote:
                    if quote != "`":
                        literals.append((content_start, index))
                    index += 1
                    break
                index += 1
            else:
                raise ValueError(
                    "generated main JavaScript has an open string literal"
                )
            continue
        index += 1
    return literals


def replace_owned_javascript_literal(
    source: str,
    basename: str,
    replacement: str,
    expected: int,
    label: str,
) -> str:
    spans = [
        (start, end)
        for start, end in quoted_javascript_literals(source)
        if source[start:end] == basename
    ]
    actual = len(spans)
    if actual != expected:
        raise ValueError(
            f"{label} basename occurrence count: expected {expected}, got {actual}"
        )
    if source.count(basename) != actual:
        raise ValueError(f"{label} basename has an unowned occurrence")
    patched = source
    for start, end in reversed(spans):
        patched = f"{patched[:start]}{replacement}{patched[end:]}"
    return patched


def prepare_output(output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    entries = list(output_dir.iterdir())
    for entry in entries:
        if entry.is_dir():
            raise ValueError(f"unexpected runtime output directory: {entry.name}")
        known = entry.name in FIXED_OUTPUTS or any(
            pattern.fullmatch(entry.name) for pattern in OUTPUT_PATTERNS
        )
        if not known:
            raise ValueError(f"unexpected runtime output file: {entry.name}")
    for entry in entries:
        entry.unlink()


def write_artifact(output_dir: Path, url: str, payload: bytes) -> None:
    if (
        Path(url).is_absolute()
        or Path(url).name != url
        or "/" in url
        or "\\" in url
    ):
        raise ValueError(f"unsafe artifact URL: {url}")
    (output_dir / url).write_bytes(payload)


def package(arguments: argparse.Namespace) -> None:
    if not DIGEST_PATTERN.fullmatch(arguments.build_id):
        raise ValueError("--build-id must be a lowercase SHA-256 digest")
    generated_dir = arguments.generated_dir.resolve(strict=True)
    generated_shape_dir = arguments.generated_shape_dir.resolve(strict=True)
    output_dir = arguments.output_dir.resolve()
    if (
        output_dir in {generated_dir, generated_shape_dir}
        or generated_dir in output_dir.parents
    ):
        raise ValueError("output directory may not contain generated inputs")
    validate_generated_shape(generated_shape_dir)

    main_source = validate_text_payload(
        file_bytes(
            generated_dir / "RhythmGameWasmProbe.js",
            "generated main JavaScript",
        ),
        "generated main JavaScript",
    )
    wasm = file_bytes(
        generated_dir / "RhythmGameWasmProbe.wasm",
        "generated Wasm",
    )
    audio_worklet = file_bytes(
        generated_dir / "RhythmGameWasmProbe.aw.js",
        "generated audio worklet",
    )
    wasm_worker = file_bytes(
        generated_dir / "RhythmGameWasmProbe.ww.js",
        "generated Wasm worker",
    )
    css = file_bytes(arguments.css, "probe CSS")
    bootstrap = file_bytes(arguments.bootstrap, "bootstrap module")
    preflight_worker = file_bytes(
        arguments.preflight_worker,
        "preflight worker",
    )
    qtloader = file_bytes(arguments.qtloader, "Qt loader")
    media = file_bytes(arguments.media, "WebM fixture")
    template = file_bytes(arguments.html_template, "HTML template").decode(
        "utf-8"
    )
    for description, payload, is_css in (
        ("generated audio worklet", audio_worklet, False),
        ("generated Wasm worker", wasm_worker, False),
        ("probe CSS", css, True),
        ("bootstrap module", bootstrap, False),
        ("preflight worker", preflight_worker, False),
        ("Qt loader", qtloader, False),
        ("HTML template", template, False),
    ):
        validate_text_payload(payload, description, css=is_css)
    icon_links = [
        link
        for link in re.findall(
            r"<link\b[^>]*>",
            template,
            flags=re.IGNORECASE,
        )
        if re.search(
            r"\brel\s*=\s*[\"']icon[\"']",
            link,
            flags=re.IGNORECASE,
        )
    ]
    if (
        template.count(FAVICON_LINK) != 1
        or icon_links != [FAVICON_LINK]
    ):
        raise ValueError("HTML favicon link contract is not exact")
    placeholders = (
        "@PROBE_CSS_URL@",
        "@PROBE_CSS_SRI@",
        "@PROBE_BOOTSTRAP_URL@",
        "@PROBE_BOOTSTRAP_SRI@",
    )
    for placeholder in placeholders:
        if template.count(placeholder) != 1:
            raise ValueError(
                f"HTML template must contain {placeholder} exactly once"
            )

    audio_url = content_name(
        "RhythmGameWasmProbe.aw", "js", audio_worklet
    )
    wasm_worker_url = content_name(
        "RhythmGameWasmProbe.ww", "js", wasm_worker
    )
    patched_main = replace_owned_javascript_literal(
        main_source,
        "RhythmGameWasmProbe.aw.js",
        audio_url,
        arguments.expected_audio_worklet_occurrences,
        "audio-worklet",
    )
    patched_main = replace_owned_javascript_literal(
        patched_main,
        "RhythmGameWasmProbe.ww.js",
        wasm_worker_url,
        arguments.expected_wasm_worker_occurrences,
        "wasm-worker",
    )
    validate_text_payload(patched_main, "patched main JavaScript")
    patched_main = patched_main.encode("utf-8")

    urls = {
        "audioWorklet": audio_url,
        "bootstrap": content_name("bootstrap", "mjs", bootstrap),
        "css": content_name("probe", "css", css),
        "mainJs": content_name(
            "RhythmGameWasmProbe", "js", patched_main
        ),
        "media": content_name("probe", "webm", media),
        "preflightWorker": content_name(
            "preflight-worker", "mjs", preflight_worker
        ),
        "qtloader": content_name("qtloader", "js", qtloader),
        "wasm": content_name("RhythmGameWasmProbe", "wasm", wasm),
        "wasmWorker": wasm_worker_url,
    }
    payloads = {
        "audioWorklet": audio_worklet,
        "bootstrap": bootstrap,
        "css": css,
        "mainJs": patched_main,
        "media": media,
        "preflightWorker": preflight_worker,
        "qtloader": qtloader,
        "wasm": wasm,
        "wasmWorker": wasm_worker,
    }
    css_sri = sri(css)
    bootstrap_sri = sri(bootstrap)
    html = (
        template.replace("@PROBE_CSS_URL@", urls["css"])
        .replace("@PROBE_CSS_SRI@", css_sri)
        .replace("@PROBE_BOOTSTRAP_URL@", urls["bootstrap"])
        .replace("@PROBE_BOOTSTRAP_SRI@", bootstrap_sri)
    ).encode("utf-8")
    if re.search(r"@[A-Z0-9_]+@", html.decode("utf-8")):
        raise ValueError("HTML template contains an unknown placeholder")
    validate_text_payload(html, "packaged HTML")

    prepare_output(output_dir)
    for role in sorted(payloads):
        write_artifact(output_dir, urls[role], payloads[role])
    write_artifact(output_dir, "RhythmGameWasmProbe.html", html)

    artifacts = {
        role: artifact(
            arguments.build_id,
            MIME[role],
            urls[role],
            payloads[role],
        )
        for role in sorted(payloads)
    }
    artifacts["html"] = artifact(
        arguments.build_id,
        MIME["html"],
        "RhythmGameWasmProbe.html",
        html,
    )
    manifest = {
        "artifacts": artifacts,
        "buildId": arguments.build_id,
        "schemaVersion": 1,
    }
    encoded_manifest = (
        json.dumps(
            manifest,
            ensure_ascii=True,
            separators=(",", ":"),
            sort_keys=True,
        )
        + "\n"
    ).encode("ascii")
    write_artifact(output_dir, "runtime-artifacts.json", encoded_manifest)


def reject_duplicate_json_keys(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def verify_html_references(html: str, artifacts: dict) -> None:
    css = artifacts["css"]
    bootstrap = artifacts["bootstrap"]
    for label, value in (
        ("CSS URL", css["url"]),
        ("CSS SRI", css["sri"]),
        ("bootstrap URL", bootstrap["url"]),
        ("bootstrap SRI", bootstrap["sri"]),
    ):
        if html.count(value) != 1:
            raise ValueError(f"packaged HTML {label} reference count is not 1")

    links = re.findall(r"<link\b[^>]*>", html, flags=re.IGNORECASE)
    scripts = re.findall(
        r"<script\b([^>]*)>(.*?)</script\s*>",
        html,
        flags=re.IGNORECASE | re.DOTALL,
    )
    if len(links) != 2 or len(scripts) != 1:
        raise ValueError("packaged HTML external resource set is not exact")
    icon_links = [
        link for link in links if 'rel="icon"' in link
    ]
    stylesheet_links = [
        link for link in links if 'rel="stylesheet"' in link
    ]
    if (
        icon_links != [FAVICON_LINK]
        or len(stylesheet_links) != 1
    ):
        raise ValueError("packaged HTML link resource set is not exact")
    link = stylesheet_links[0]
    script_attributes, script_body = scripts[0]
    for value in (
        'rel="stylesheet"',
        f'href="{css["url"]}"',
        f'integrity="{css["sri"]}"',
        'crossorigin="anonymous"',
    ):
        if value not in link:
            raise ValueError("packaged HTML CSS reference is not exact")
    for value in (
        'type="module"',
        f'src="{bootstrap["url"]}"',
        f'integrity="{bootstrap["sri"]}"',
        'crossorigin="anonymous"',
    ):
        if value not in script_attributes:
            raise ValueError("packaged HTML bootstrap reference is not exact")
    if script_body.strip() or re.search(r"<style\b", html, re.IGNORECASE):
        raise ValueError("packaged HTML contains inline executable content")


def verify_output(output_argument: Path, expected_build_id: str) -> None:
    if not DIGEST_PATTERN.fullmatch(expected_build_id):
        raise ValueError(
            "--expected-build-id must be a lowercase SHA-256 digest"
        )
    if output_argument.is_symlink():
        raise ValueError("runtime output directory may not be a link")
    output_dir = output_argument.resolve(strict=True)
    if not output_dir.is_dir():
        raise ValueError(f"missing runtime output directory: {output_dir}")

    entries = list(output_dir.iterdir())
    for entry in entries:
        if entry.is_symlink() or not entry.is_file():
            raise ValueError(
                f"runtime output entry is not a regular file: {entry.name}"
            )
    manifest_path = output_dir / "runtime-artifacts.json"
    manifest_bytes = file_bytes(manifest_path, "runtime artifact manifest")
    try:
        manifest = json.loads(
            manifest_bytes.decode("utf-8"),
            object_pairs_hook=reject_duplicate_json_keys,
        )
    except json.JSONDecodeError as error:
        raise ValueError(f"invalid runtime artifact manifest JSON: {error}")
    if (
        not isinstance(manifest, dict)
        or set(manifest) != {"artifacts", "buildId", "schemaVersion"}
        or manifest.get("schemaVersion") != 1
        or manifest.get("buildId") != expected_build_id
        or not isinstance(manifest.get("artifacts"), dict)
        or set(manifest["artifacts"]) != set(MIME)
    ):
        raise ValueError("runtime artifact manifest shape or build ID drifted")
    canonical = (
        json.dumps(
            manifest,
            ensure_ascii=True,
            separators=(",", ":"),
            sort_keys=True,
        )
        + "\n"
    ).encode("ascii")
    if manifest_bytes != canonical:
        raise ValueError("runtime artifact manifest is not canonical JSON")

    expected_entries = {"runtime-artifacts.json"}
    folded_urls = set()
    payloads = {}
    validated_artifacts = {}
    for role in sorted(MIME):
        artifact_entry = manifest["artifacts"][role]
        if (
            not isinstance(artifact_entry, dict)
            or set(artifact_entry) != {
                "buildId",
                "bytes",
                "mime",
                "sha256",
                "sri",
                "url",
            }
        ):
            raise ValueError(f"runtime artifact {role} shape drifted")
        digest = artifact_entry["sha256"]
        url = artifact_entry["url"]
        byte_length = artifact_entry["bytes"]
        if (
            artifact_entry["buildId"] != expected_build_id
            or artifact_entry["mime"] != MIME[role]
            or not isinstance(digest, str)
            or not DIGEST_PATTERN.fullmatch(digest)
            or not isinstance(artifact_entry["sri"], str)
            or not SRI_PATTERN.fullmatch(artifact_entry["sri"])
            or not isinstance(url, str)
            or url != expected_artifact_url(role, digest)
            or not isinstance(byte_length, int)
            or isinstance(byte_length, bool)
            or byte_length < 0
        ):
            raise ValueError(f"runtime artifact {role} metadata drifted")
        folded_url = url.casefold()
        if folded_url in folded_urls:
            raise ValueError(f"duplicate runtime artifact URL: {url}")
        folded_urls.add(folded_url)
        expected_entries.add(url)
        validated_artifacts[role] = artifact_entry

    actual_entries = {entry.name for entry in entries}
    if actual_entries != expected_entries:
        missing = sorted(expected_entries - actual_entries)
        extra = sorted(actual_entries - expected_entries)
        raise ValueError(
            "runtime directory set drifted"
            f"; missing={missing}; extra={extra}"
        )
    for role, artifact_entry in validated_artifacts.items():
        payload = file_bytes(
            output_dir / artifact_entry["url"],
            f"runtime artifact {role}",
        )
        if (
            len(payload) != artifact_entry["bytes"]
            or sha256(payload) != artifact_entry["sha256"]
            or sri(payload) != artifact_entry["sri"]
        ):
            raise ValueError(f"runtime artifact {role} bytes drifted")
        if role in TEXT_ROLES:
            validate_text_payload(
                payload,
                f"runtime artifact {role}",
                css=role == "css",
            )
        payloads[role] = payload
    html = payloads["html"].decode("utf-8")
    verify_html_references(html, manifest["artifacts"])


def require_package_arguments(arguments: argparse.Namespace) -> None:
    required = (
        "build_id",
        "generated_dir",
        "generated_shape_dir",
        "output_dir",
        "html_template",
        "css",
        "bootstrap",
        "preflight_worker",
        "qtloader",
        "media",
        "expected_audio_worklet_occurrences",
        "expected_wasm_worker_occurrences",
    )
    missing = [
        f"--{name.replace('_', '-')}"
        for name in required
        if getattr(arguments, name) is None
    ]
    if missing:
        raise ValueError(
            "packaging mode is missing required arguments: "
            + ", ".join(missing)
        )


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(
        description="Package the content-addressed Gate 1B runtime"
    )
    result.add_argument("--build-id")
    result.add_argument("--generated-dir", type=Path)
    result.add_argument("--generated-shape-dir", type=Path)
    result.add_argument("--output-dir", type=Path)
    result.add_argument("--html-template", type=Path)
    result.add_argument("--css", type=Path)
    result.add_argument("--bootstrap", type=Path)
    result.add_argument("--preflight-worker", type=Path)
    result.add_argument("--qtloader", type=Path)
    result.add_argument("--media", type=Path)
    result.add_argument(
        "--expected-audio-worklet-occurrences",
        type=int,
    )
    result.add_argument(
        "--expected-wasm-worker-occurrences",
        type=int,
    )
    result.add_argument("--verify-output-dir", type=Path)
    result.add_argument("--expected-build-id")
    return result


def main() -> int:
    try:
        arguments = parser().parse_args()
        if arguments.verify_output_dir is not None:
            if arguments.expected_build_id is None:
                raise ValueError(
                    "verification mode requires --expected-build-id"
                )
            verify_output(
                arguments.verify_output_dir,
                arguments.expected_build_id,
            )
        else:
            if arguments.expected_build_id is not None:
                raise ValueError(
                    "--expected-build-id requires --verify-output-dir"
                )
            require_package_arguments(arguments)
            package(arguments)
    except (OSError, UnicodeError, ValueError) as error:
        print(f"package_runtime_artifacts.py: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
