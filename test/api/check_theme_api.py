"""Compare generated theme types and documented QML members with the API baseline.

Run after building RhythmGame_qml. Use --update only for a reviewed API change.
This records declarations, not runtime behavior or native enum numeric values.
"""

import argparse
import difflib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
BASELINE = ROOT / "docs/api/theme-api-1.0.json"
INTERNAL = {"FontResolver", "rhythm_game_qml::ScreenContexts"}
NAMESPACES = (
    "arena::", "gameplay_logic::", "input::", "qml_components::",
    "resource_managers::", "rhythm_game_qml::", "sounds::", "support::",
)


def parse_types(text):
    """Read the small declarative format emitted by qmltyperegistrar."""
    text = text[text.index("Module {"):]
    tokens = re.findall(r'"(?:\\.|[^"\\])*"|[A-Za-z_][\w.]*|-?\d+|[{}\[\]:;,]', text)
    index = 0

    def take():
        nonlocal index
        token = tokens[index]
        index += 1
        return token

    def value():
        token = take()
        if token == "[":
            items = []
            while tokens[index] != "]":
                if tokens[index] == ",":
                    take()
                else:
                    items.append(value())
            take()
            return items
        return json.loads(token)

    def body():
        result = {"members": []}
        while tokens[index] != "}":
            name = take()
            if name == ";":
                continue
            separator = take()
            if separator == ":":
                result[name] = value()
            elif separator == "{":
                result["members"].append({"kind": name, **body()})
            else:
                raise ValueError(f"Unexpected token after {name}: {separator}")
        take()
        return result

    assert take() == "Module"
    assert take() == "{"
    return body()["members"]


def clean(node):
    result = {}
    for key, value in node.items():
        if key in {"file", "lineNumber", "index", "read", "write", "exportMetaObjectRevisions"}:
            continue
        if key == "members":
            members = [clean(member) for member in value if member.get("access") != "private"]
            if members:
                # Parameter order is part of a method's signature.
                result[key] = members if all(m["kind"] == "Parameter" for m in members) else sorted(members, key=lambda m: (m["kind"], m.get("name", ""), json.dumps(m, sort_keys=True)))
        else:
            result[key] = True if key == "reset" else value
    return result


def snapshot(qmltypes):
    native = {}
    for node in parse_types(qmltypes.read_text(encoding="utf-8")):
        name = node["name"]
        if name in INTERNAL or not (name == "Rg" or name.startswith(NAMESPACES)):
            continue
        if name == "gameplay_logic::CourseRunner":
            continue
        if name == "gameplay_logic::ChartRunner":
            node["members"] = [m for m in node["members"] if m["kind"] == "Enum"]
        if name == "qml_components::ChartLoader":
            node["members"] = [m for m in node["members"] if m.get("name") in {"loadChartData", "loadChartDataFromDb"}]
        native[name] = clean(node)
    if "rhythm_game_qml::GameplayContext" not in native or "sounds::AudioPlayer" not in native:
        raise ValueError("QML metadata is missing supported types. Rebuild RhythmGame_qml first.")

    qml = {}
    files = list((ROOT / "RhythmGameQml").glob("Standard*.qml"))
    files += [ROOT / "RhythmGameQml" / name for name in ("ScoreDetails.qml", "PendingReplyGroup.qml")]
    declaration = re.compile(r'/\*![\s\S]*?\*/\s*((?:(?:readonly|required)\s+)?property\s+[\w<>]+\s+\w+|function\s+\w+\([^\n]*?\)(?:\s*:\s*\w+)?|signal\s+\w+\([^\n]*?\))')
    for path in files:
        source = path.read_text(encoding="utf-8")
        qml[path.stem] = sorted(" ".join(m.split()) for m in declaration.findall(source))

    # Role strings are public even where the native role-number enum is private.
    roles = {}
    for folder in ("src/arena", "src/qml_components", "src/resource_managers", "src/gameplay_logic", "src/input"):
        for path in (ROOT / folder).glob("*.cpp"):
            source = path.read_text(encoding="utf-8")
            for match in re.finditer(r'(\w+)::roleNames\([^)]*\)[^{]*\{', source):
                start = match.end()
                depth, end = 1, start
                while depth and end < len(source):
                    depth += (source[end] == "{") - (source[end] == "}")
                    end += 1
                roles[match[1]] = sorted(set(re.findall(r'"([A-Za-z_]\w*)"', source[start:end])))

    host = (ROOT / "RhythmGameQml/ContentFrame.qml").read_text(encoding="utf-8")
    operations = re.findall(r'^        function (.+)\s*\{\s*$', host, re.MULTILINE)
    return {"apiVersion": "1.0", "nativeTypes": dict(sorted(native.items())),
            "qmlMembers": dict(sorted(qml.items())), "modelRoles": dict(sorted(roles.items())),
            "globalRoot": {"properties": ["readonly currentScreen"], "operations": sorted(s.strip() for s in operations)}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qmltypes", required=True, type=Path)
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()
    actual = json.dumps(snapshot(args.qmltypes), ensure_ascii=False, indent=2) + "\n"
    if args.update:
        BASELINE.parent.mkdir(parents=True, exist_ok=True)
        BASELINE.write_text(actual, encoding="utf-8")
        print("Updated", BASELINE.relative_to(ROOT))
        return
    expected = BASELINE.read_text(encoding="utf-8")
    if actual != expected:
        print("".join(difflib.unified_diff(expected.splitlines(True), actual.splitlines(True), fromfile="API baseline", tofile="Current API")))
        raise SystemExit(1)
    print("Theme API declarations match the baseline.")


if __name__ == "__main__":
    main()
