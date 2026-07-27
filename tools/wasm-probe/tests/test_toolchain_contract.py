import hashlib
import json
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PROBE = REPO / "tools" / "wasm-probe"
BASELINE = "a0400024711b283056538ac19ced80b91a83c24c"
EMSDK_COMMIT = "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
FULL_QT_SHA256 = (
    "252acef8c5ae68074d91cadba2ee4a83465051bbb970dd26e8f0daa0f3904e03"
)
QTBASE_SHA256 = (
    "d9594a31228aa23ad6b531719a29b45f0f3989fe6c136d45767ea179f233c1ac"
)
QTBASE_SHA512 = (
    "b5608b6cefd483ecdc5e4fa3536acfc31116c8dfb698257f945180a8e412ee44"
    "4dc670d754d4f6145649170f7d55637a70820188337a6c6b79193fbfbcd6a3fc"
)
QTBASE_VCPKG_TREE = "29a7f9f115d568b271a3b99fabeac886ec248f9f"
QTBASE_WASM_PATCH_SHA256 = (
    "8aa3ed93e30f16c3c9691b35dee1f27c9608bcf424fc4b0e86009ce64709c786"
)
QTBASE_WASM_EVENT_PATH_PATCH_SHA256 = (
    "f67e8e1eda7dc7abd208d113b3bc4057bbff7d45d79fdebd133c3fde3c0466ec"
)
QTMULTIMEDIA_SOURCE_SHA256 = (
    "390f8e52ddee3aca5c4de7eead900c84c4fa61ff6d1f0ebea9c7543365c09b0a"
)
QTMULTIMEDIA_VCPKG_SHA512 = (
    "accd9534b96df8ff464c219bf579c3da43e30b33efb35fc7e311c9e133cb5725"
    "2f433422ca917f83b738e67cf925b19e0847ffcfaa92fe0e3d21b603dd580cd5"
)
QTMULTIMEDIA_WASM_PATCH_SHA256 = (
    "6237459c2301f2e629d686d974feeb759cf392c0ef653363f18f767f639aaa24"
)
QTMULTIMEDIA_WASM_PATH_MAP_PATCH_SHA256 = (
    "31f3610dcae306e6c0e49169077aaa7b5a7e704562aaa0f07ba78fe37d726630"
)
QTMULTIMEDIA_WASM_MEDIA_LIFECYCLE_PATCH_SHA256 = (
    "7459fdd7a1982099fba9b84cdeb69833a5b0a820ec1d146bcdecc892241bd182"
)
QTMULTIMEDIA_WASM_UPSTREAM_COMMITS = [
    "7f71286a9f22ae69936a21b561570c4ea1af2431",
    "35b0ea686685cb591d598d503bfa110daf6c69e2",
    "6cedb5d96f36c5d406d1bb58352dc05523c62fa4",
    "9018988e854ce6b7689e716b4afe93d6173a135c",
    "191cda01b86bc6b28e663426bb7a3eef6b2d39cc",
    "4e61fa7da7e7db730e6e4762839de24f99e7803c",
    "8a2093d1dda70eae63a0522537c93605f8932041",
    "d6f64920e9024de8cf0d8761f304fc8999700783",
]
QTDECLARATIVE_PATCH_SHA256 = (
    "2A015242AF462BE117A2924D4D8DB2C753B29891921E714C23BF1AB4355C4C50"
)
CMAKE_WINDOWS_X64_SHA256 = (
    "eb4ebf5155dbb05436d675706b2a08189430df58904257ae5e91bcba4c86933c"
)
NINJA_WINDOWS_SHA256 = (
    "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65"
)
CMAKE_EXECUTABLE_SHA256 = (
    "daae341e73330c9c5bf391f22d10510c0f70e5f01d2b61b0fd256cd9edd28379"
)
NINJA_EXECUTABLE_SHA256 = (
    "e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8"
)
HOST_COMPILER_SHA256 = (
    "9cf613fcbebece019712511eec4b1a32d0a9c94e65249a1aeb326305c2388d6b"
)
SOURCE_DATE_EPOCH = 1782488244
REPRODUCIBLE_BUILD_DERIVATION = (
    "vcpkg-baseline-source-archive-root-entry-utc"
)


class ToolchainContractTest(unittest.TestCase):
    def test_lock_file_has_exact_production_pins(self) -> None:
        lock = json.loads((PROBE / "toolchain-lock.json").read_text("utf-8"))
        self.assertEqual(lock["qt"]["version"], "6.11.1")
        self.assertEqual(lock["emscripten"]["version"], "4.0.7")
        self.assertEqual(lock["emscripten"]["emsdkCommit"], EMSDK_COMMIT)
        self.assertEqual(
            lock["reproducibleBuild"],
            {
                "sourceDateEpoch": SOURCE_DATE_EPOCH,
                "derivation": REPRODUCIBLE_BUILD_DERIVATION,
                "vcpkgMaxConcurrency": 8,
            },
        )
        self.assertEqual(
            lock["emscripten"]["releaseHash"],
            "ef4e9cedeac3332e4738087567552063f4f250d3",
        )
        self.assertEqual(
            lock["emscripten"]["releaseManifest"]["path"],
            "emscripten-releases-tags.json",
        )
        self.assertEqual(
            lock["emscripten"]["payload"]["algorithm"],
            "sha256-path-null-digest-lf-v1",
        )
        self.assertEqual(
            lock["emscripten"]["generatedBytecode"],
            {
                "cacheDirectory": "__pycache__",
                "fileSuffix": ".pyc",
                "normalization": (
                    "authenticate-non-bytecode-delete-cache-"
                    "authenticate-full-v1"
                ),
            },
        )
        self.assertEqual(lock["emscripten"]["bootstrapScript"], "emsdk.py")
        self.assertRegex(
            lock["emscripten"]["bootstrapScriptSha256"],
            r"^[0-9a-f]{64}$",
        )
        self.assertEqual(
            lock["emscripten"]["bootstrapPython"][
                "installationDirectory"
            ],
            "python/3.9.2-nuget_64bit",
        )
        self.assertEqual(
            lock["emscripten"]["bootstrapPython"]["executable"],
            "python.exe",
        )
        self.assertEqual(
            lock["emscripten"]["bootstrapPython"]["payload"]["fileCount"],
            1486,
        )
        self.assertEqual(
            lock["emscripten"]["driverApi"]["pythonImportClosure"],
            {
                "algorithm": "sha256-path-null-digest-lf-v1",
                "fileCount": 225,
                "totalBytes": 3159759,
                "inventorySha256": (
                    "acdbf9111c4779b62764eaa595a184179f7ffe8f46de07ea"
                    "4303f635d1308477"
                ),
                "aggregateSha256": (
                    "4a378899a0a3ad36e19f7f5e0170641fdb647d61aaf464c"
                    "721b07d729abdf6f6"
                ),
            },
        )
        self.assertRegex(
            lock["emscripten"]["nodeExecutableSha256"],
            r"^[0-9a-f]{64}$",
        )
        self.assertEqual(
            lock["emscripten"]["sourceArchive"]["allowedRuntimePrefixes"],
            [
                "downloads",
                "node/20.18.0_64bit",
                "python/3.9.2-nuget_64bit",
                "upstream/bin",
                "upstream/emscripten",
                "upstream/lib",
            ],
        )
        self.assertEqual(
            lock["vcpkg"]["sourceArchive"]["allowedRuntimePrefixes"],
            [],
        )
        self.assertNotIn(".git", json.dumps(lock["emscripten"]["sourceArchive"]))
        self.assertNotIn(".git", json.dumps(lock["vcpkg"]["sourceArchive"]))
        gate_scripts = {
            "adapterSha256": (
                PROBE / "scripts" / "invoke_emscripten_driver.py"
            ),
            "responseAuditorSha256": (
                PROBE / "scripts" / "audit_emscripten_response_files.py"
            ),
        }
        for field, path in gate_scripts.items():
            self.assertEqual(
                lock["gateTools"][field],
                hashlib.sha256(path.read_bytes()).hexdigest(),
            )
        self.assertEqual(
            lock["emscripten"]["payload"]["excludedSegments"],
            [],
        )
        self.assertEqual(
            lock["emscripten"]["payload"]["excludedSuffixes"],
            [],
        )
        self.assertGreater(lock["emscripten"]["payload"]["fileCount"], 10_000)
        for name in ("inventorySha256", "aggregateSha256"):
            self.assertRegex(
                lock["emscripten"]["payload"][name],
                r"^[0-9a-f]{64}$",
            )
        self.assertEqual(lock["vcpkg"]["baseline"], BASELINE)
        self.assertEqual(lock["qt"]["fullQtSourceSha256"], FULL_QT_SHA256)
        self.assertEqual(lock["qt"]["qtbaseSourceSha256"], QTBASE_SHA256)
        self.assertEqual(lock["qt"]["qtbaseVcpkgSha512"], QTBASE_SHA512)
        self.assertEqual(lock["qt"]["qtbaseVcpkgTree"], QTBASE_VCPKG_TREE)
        self.assertEqual(
            lock["qt"]["qtbaseWasmPatchSha256"],
            QTBASE_WASM_PATCH_SHA256,
        )
        self.assertEqual(
            lock["qt"]["qtbaseWasmEventPathPatchSha256"],
            QTBASE_WASM_EVENT_PATH_PATCH_SHA256,
        )
        self.assertEqual(
            lock["qt"]["qtmultimediaSourceSha256"],
            QTMULTIMEDIA_SOURCE_SHA256,
        )
        self.assertEqual(
            lock["qt"]["qtmultimediaVcpkgSha512"],
            QTMULTIMEDIA_VCPKG_SHA512,
        )
        self.assertEqual(
            lock["qt"]["qtmultimediaWasmPatchSha256"],
            QTMULTIMEDIA_WASM_PATCH_SHA256,
        )
        self.assertEqual(
            lock["qt"]["qtmultimediaWasmPathMapPatchSha256"],
            QTMULTIMEDIA_WASM_PATH_MAP_PATCH_SHA256,
        )
        self.assertEqual(
            lock["qt"]["qtmultimediaWasmMediaLifecyclePatchSha256"],
            QTMULTIMEDIA_WASM_MEDIA_LIFECYCLE_PATCH_SHA256,
        )
        self.assertEqual(
            lock["qt"]["qtmultimediaWasmUpstreamCommits"],
            QTMULTIMEDIA_WASM_UPSTREAM_COMMITS,
        )
        multimedia_binary = lock["qt"]["qtmultimediaBinaryPackage"]
        self.assertEqual(
            set(multimedia_binary),
            {
                "abi",
                "algorithm",
                "aggregateSha256",
                "deterministicPayload",
                "installableFileCount",
                "installableInventorySha256",
                "memberCount",
                "members",
                "totalBytes",
            },
        )
        self.assertEqual(
            multimedia_binary["abi"],
            "0924ac18c8d388a9cf2afa92b38769923d8247723a80f4fcf3ea8074951835e5",
        )
        self.assertEqual(
            multimedia_binary["algorithm"],
            "sha256-path-null-bytes-null-digest-lf-v1",
        )
        self.assertEqual(multimedia_binary["memberCount"], 16)
        self.assertEqual(multimedia_binary["totalBytes"], 6226879)
        self.assertEqual(multimedia_binary["installableFileCount"], 367)
        self.assertRegex(
            multimedia_binary["installableInventorySha256"],
            r"^[0-9a-f]{64}$",
        )
        deterministic = multimedia_binary["deterministicPayload"]
        self.assertEqual(
            set(deterministic),
            {
                "algorithm",
                "aggregateSha256",
                "excluded",
                "fileCount",
                "inventorySha256",
                "totalBytes",
            },
        )
        self.assertEqual(
            deterministic["algorithm"],
            "sha256-path-null-bytes-null-digest-lf-v1",
        )
        self.assertEqual(
            deterministic["excluded"],
            [
                "sbom/qtmultimedia-6.11.1.spdx",
                "share/qtmultimedia/vcpkg.spdx.json",
                "share/qtmultimedia/vcpkg_abi_info.txt",
            ],
        )
        self.assertGreater(deterministic["fileCount"], 300)
        self.assertGreater(deterministic["totalBytes"], 1_000_000)
        for name in ("aggregateSha256", "inventorySha256"):
            self.assertRegex(deterministic[name], r"^[0-9a-f]{64}$")
        members = multimedia_binary["members"]
        self.assertEqual(
            [member["path"] for member in members],
            sorted(member["path"] for member in members),
        )
        aggregate = hashlib.sha256()
        for member in members:
            self.assertEqual(
                set(member),
                {"bytes", "path", "sha256"},
            )
            self.assertGreater(member["bytes"], 0)
            self.assertRegex(member["sha256"], r"^[0-9a-f]{64}$")
            aggregate.update(member["path"].encode("utf-8"))
            aggregate.update(b"\0")
            aggregate.update(str(member["bytes"]).encode("ascii"))
            aggregate.update(b"\0")
            aggregate.update(member["sha256"].encode("ascii"))
            aggregate.update(b"\n")
        self.assertEqual(
            aggregate.hexdigest(),
            multimedia_binary["aggregateSha256"],
        )
        multimedia_core = next(
            member
            for member in members
            if member["path"] == "lib/libQt6Multimedia.a"
        )
        self.assertEqual(
            multimedia_core["sha256"],
            "40ea9f62c037a8fc75e8470ae9f90f9c455c5b2d5fef4d60dbeeb5ca8812bea3",
        )
        resonance = next(
            member
            for member in members
            if member["path"] == "lib/libQt6BundledResonanceAudio.a"
        )
        self.assertEqual(resonance["bytes"], 1236058)
        self.assertEqual(
            resonance["sha256"],
            "e87b23d9e602e1935b5ec221ec21256de73d35e620173f7ad0fb3917cb287dcd",
        )
        multimedia_compile = lock["qt"]["qtmultimediaCompileCommands"]
        self.assertEqual(
            multimedia_compile,
            {
                "aggregateSha256": (
                    "c9df99b9b791da08920d68effd6211e6d475cea789b7d687b2164b51eafad955"
                ),
                "algorithm": (
                    "sha256-canonical-qtmultimedia-compile-commands-json-v2"
                ),
                "cCommandCount": 1,
                "commandCount": 222,
                "cxxCommandCount": 221,
                "installedObjectAggregateSha256": (
                    "f46f0c066ef3d89cddafbe16aef173e63c1c00032fce229dcaf7e6e623e3b588"
                ),
                "installedObjectOccurrenceCount": 217,
                "ninjaParity": True,
                "objectOutputCount": 217,
                "outputInventorySha256": (
                    "00e1be2961daa565ac5b60b9698c6c94e9c3f54005d8bb4ab64fff7863839d53"
                ),
                "pathMapTargets": {
                    "build": "/qt/qtmultimedia/build",
                    "source": "/qt/qtmultimedia/source",
                },
                "perEdgeObjectDigestCount": 217,
                "pchByteIdentity": (
                    "excluded-noninstallable-root-dependent-v1"
                ),
                "pchOutputCount": 5,
            },
        )
        multimedia_reproducibility = (
            lock["qt"]["qtmultimediaReproducibility"]
        )
        self.assertEqual(
            set(multimedia_reproducibility),
            {
                "abi",
                "algorithm",
                "allCompileCommandsCanonicalIdentical",
                "allInstallableFilesContractVerified",
                "allStaticMembersByteIdentical",
                "buildRootLengthDelta",
                "compileAggregateSha256",
                "compileCommandCount",
                "deterministicPayloadAggregateSha256",
                "installableFileCount",
                "manifestDerivation",
                "primaryBuiltinBaseline",
                "primaryManifestSha256",
                "secondaryAbiInfoFile",
                "secondaryBinaryCacheFile",
                "secondaryBinaryCacheRoot",
                "secondaryBinarySourcePolicy",
                "secondaryBuildRoot",
                "secondaryBuildtreesRoot",
                "secondaryDownloadsRoot",
                "secondaryFeatureFlags",
                "secondaryInstalledRoot",
                "secondaryManifestFile",
                "secondaryManifestSha256",
                "secondaryOuterCMakeFile",
                "secondaryOverlayPorts",
                "secondaryOverlayTriplets",
                "secondaryPackagesRoot",
                "secondaryPortInstallPrefix",
                "secondaryProbeBuildRoot",
                "secondaryTargetTriplet",
                "secondaryHostTriplet",
                "secondaryToolchainFile",
                "secondaryChainloadToolchainFile",
                "sourceRootLengthDelta",
                "staticAggregateSha256",
                "staticMemberCount",
            },
        )
        self.assertEqual(
            multimedia_reproducibility["abi"],
            multimedia_binary["abi"],
        )
        self.assertEqual(
            multimedia_reproducibility["compileAggregateSha256"],
            multimedia_compile["aggregateSha256"],
        )
        self.assertEqual(
            multimedia_reproducibility["staticAggregateSha256"],
            multimedia_binary["aggregateSha256"],
        )
        self.assertEqual(
            multimedia_reproducibility["primaryBuiltinBaseline"],
            BASELINE,
        )
        self.assertEqual(
            multimedia_reproducibility["secondaryFeatureFlags"],
            "-versions",
        )
        self.assertEqual(
            multimedia_reproducibility["buildRootLengthDelta"],
            72,
        )
        self.assertEqual(
            multimedia_reproducibility["sourceRootLengthDelta"],
            72,
        )
        self.assertTrue(
            multimedia_reproducibility[
                "allCompileCommandsCanonicalIdentical"
            ]
        )
        self.assertTrue(
            multimedia_reproducibility[
                "allInstallableFilesContractVerified"
            ]
        )
        self.assertTrue(
            multimedia_reproducibility["allStaticMembersByteIdentical"]
        )
        self.assertEqual(
            lock["qt"]["targetStaticLinkInputs"],
            {
                "aggregateSha256": (
                    "c40460508e292f56c91331e655b9cf34023f0cbccfad1054a35ac8bc16641bff"
                ),
                "algorithm": "sha256-path-null-digest-lf-v1",
                "fileCount": 385,
                "inventorySha256": (
                    "9b477434a32545379aafd3374623791afeb3f8d832a69f1a03e125eb427ab4e2"
                ),
                "totalBytes": 192344397,
            },
        )
        self.assertEqual(
            lock["buildTools"]["cmake"],
            {
                "version": "4.2.3",
                "url": (
                    "https://cmake.org/files/v4.2/"
                    "cmake-4.2.3-windows-x86_64.zip"
                ),
                "sha256": CMAKE_WINDOWS_X64_SHA256,
                "archiveFile": "cmake-4.2.3-windows-x86_64.zip",
                "executableSha256": CMAKE_EXECUTABLE_SHA256,
                "directory": "cmake-4.2.3-windows-x86_64",
                "payload": {
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "stripPrefix": "cmake-4.2.3-windows-x86_64",
                    "fileCount": 8525,
                    "directoryCount": 148,
                    "totalBytes": 132316585,
                    "inventorySha256": (
                        "48f746fb6ca853fd693c018eaa3220da06b4251275a15a3e41a"
                        "93bc1013c3dd3"
                    ),
                    "directoryInventorySha256": (
                        "649f6d95061ef02fe899c0c7062b551e401e372a098c9a9880"
                        "cbbee4d72fc814"
                    ),
                    "aggregateSha256": (
                        "38d0389fbb638f78ec2a624592d331efeef3c3f951eb3a3aff1"
                        "eacbff16146f7"
                    ),
                },
            },
        )
        self.assertEqual(
            lock["buildTools"]["ninja"],
            {
                "version": "1.13.2",
                "url": (
                    "https://github.com/ninja-build/ninja/releases/"
                    "download/v1.13.2/ninja-win.zip"
                ),
                "sha256": NINJA_WINDOWS_SHA256,
                "archiveFile": "ninja-win.zip",
                "executableSha256": NINJA_EXECUTABLE_SHA256,
                "directory": "ninja-1.13.2-win",
                "payload": {
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "stripPrefix": "",
                    "fileCount": 1,
                    "directoryCount": 0,
                    "totalBytes": 603648,
                    "inventorySha256": (
                        "014cb71e5fd86a18adb79f57e26acbd58577f6f84a52491297a"
                        "3454a3b38f477"
                    ),
                    "directoryInventorySha256": (
                        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495"
                        "991b7852b855"
                    ),
                    "aggregateSha256": (
                        "90779f6fe330259113900c836a8dbf973c87905e098e0ada0121"
                        "b5ac135d6dac"
                    ),
                },
            },
        )
        self.assertEqual(
            lock["hostCompiler"],
            {
                "basename": "cl.exe",
                "toolsetRelativePath": (
                    "VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe"
                ),
                "cmakeCompilerId": "MSVC",
                "cmakeCompilerVersion": "19.51.36244.0",
                "frontendVariant": "MSVC",
                "architecture": "x64",
                "platform": "Windows",
                "executableSha256": HOST_COMPILER_SHA256,
            },
        )
        self.assertEqual(
            lock["qt"]["qtdeclarativePatchSha256"],
            QTDECLARATIVE_PATCH_SHA256,
        )

    def test_probe_has_real_adapter_authenticated_c_compile_and_link_edges(
        self,
    ) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        manifest = (PROBE / "input-manifest.txt").read_text("utf-8")
        self.assertIn(
            "project(RhythmGameWasmProbe VERSION 0.1.0 LANGUAGES C CXX)",
            cmake,
        )
        self.assertIn("set(CMAKE_C_COMPILER_LAUNCHER", cmake)
        self.assertIn("set(CMAKE_C_LINKER_LAUNCHER", cmake)
        self.assertIn("add_executable(RhythmGameWasmCLauncherProbe", cmake)
        self.assertIn("src/LauncherProbe.c", cmake)
        self.assertRegex(
            cmake,
            (
                r"set_target_properties\(\s*"
                r"RhythmGameWasmCLauncherProbe\s+PROPERTIES"
                r"(?s:.*?)AUTOMOC OFF"
                r"(?s:.*?)LINKER_LANGUAGE C"
            ),
        )
        self.assertIn("tools/wasm-probe/src/LauncherProbe.c\n", manifest)

    def test_probe_input_checkout_byte_policy_is_bound_and_portable(
        self,
    ) -> None:
        entries = (
            PROBE / "input-manifest.txt"
        ).read_text("utf-8").splitlines()
        attributes = (
            REPO / ".gitattributes"
        ).read_text("utf-8").splitlines()
        self.assertEqual(len(entries), 113)
        self.assertEqual(entries.count(".gitattributes"), 1)
        self.assertEqual(
            entries.count(
                "tools/wasm-probe/tests/test_toolchain_contract.py"
            ),
            1,
        )
        self.assertEqual(
            entries.count("tools/wasm-probe/toolchain-lock.json"),
            1,
        )
        self.assertEqual(
            entries.count(
                "vcpkgOverlayPortsWasm/qtbase/"
                "preserve-wasm-event-composed-path.patch"
            ),
            1,
        )
        for path in (
            "canonicalize-wasm-build-paths.patch",
            "correct-wasm-media-lifecycle.patch",
            "defer-wasm-media-device-notifications.patch",
            "port.data.cmake",
            "portfile.cmake",
            "vcpkg.json",
        ):
            self.assertEqual(
                entries.count(
                    f"vcpkgOverlayPortsWasm/qtmultimedia/{path}"
                ),
                1,
            )
        for rule in (
            "/.gitattributes text eol=lf",
            "cmake/toolchains/vcpkg-emscripten.cmake text eol=lf",
            "tools/wasm-probe/** text eol=lf",
            "vcpkgOverlayPorts/qtdeclarative/* text eol=lf",
            "vcpkgTriplets/*.cmake text eol=lf",
            (
                "vcpkgOverlayPorts/qtdeclarative/"
                "24205cd-qquickwindow-child-window-stacking.patch -text"
            ),
            "vcpkgOverlayPortsWasm/qtbase/** -text -whitespace",
            "vcpkgOverlayPortsWasm/qtmultimedia/** -text -whitespace",
        ):
            self.assertEqual(attributes.count(rule), 1)

    def test_manifest_is_isolated_and_has_no_native_only_features(self) -> None:
        manifest = json.loads((PROBE / "vcpkg.json").read_text("utf-8"))
        self.assertEqual(manifest["builtin-baseline"], BASELINE)
        encoded = json.dumps(manifest, sort_keys=True)
        for forbidden in (
            "ffmpeg",
            "qtkeychain",
            "qtinterfaceframework",
            "openimageio",
            "sdl2-image",
            "vulkan",
            "openssl",
            "dnslookup",
            "llfio",
            "mimalloc",
        ):
            self.assertNotIn(forbidden, encoded.lower())
        self.assertIn("qtbase", encoded)
        self.assertIn("qtdeclarative", encoded)
        self.assertIn("qtimageformats", encoded)
        self.assertIn("qtmultimedia", encoded)
        self.assertIn("qtwebsockets", encoded)
        qtbase = next(
            dependency
            for dependency in manifest["dependencies"]
            if isinstance(dependency, dict)
            and dependency.get("name") == "qtbase"
        )
        self.assertIn("freetype", qtbase["features"])
        self.assertIn("gles2", qtbase["features"])
        self.assertIn("opengl", qtbase["features"])
        qttools = next(
            dependency
            for dependency in manifest["dependencies"]
            if isinstance(dependency, dict)
            and dependency.get("name") == "qttools"
        )
        self.assertTrue(qttools["host"])
        self.assertFalse(qttools["default-features"])
        self.assertEqual(qttools["features"], ["linguist"])

    def test_target_triplet_is_static_and_uniform(self) -> None:
        triplet = (
            REPO / "vcpkgTriplets" / "wasm32-emscripten-rg.cmake"
        ).read_text("utf-8")
        self.assertIn("set(VCPKG_TARGET_ARCHITECTURE wasm32)", triplet)
        self.assertIn("set(VCPKG_LIBRARY_LINKAGE static)", triplet)
        self.assertIn("set(VCPKG_BUILD_TYPE release)", triplet)
        self.assertIn("set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)", triplet)
        self.assertIn("EXPECTED_EMSCRIPTEN_VERSION \"4.0.7\"", triplet)
        self.assertIn("EMSDK_PYTHON", triplet)
        self.assertIn(
            'set(VCPKG_C_FLAGS "-pthread -sSUPPORT_LONGJMP=wasm")',
            triplet,
        )
        self.assertIn(
            "set(VCPKG_CXX_FLAGS "
            '"-pthread -fwasm-exceptions -sSUPPORT_LONGJMP=wasm")',
            triplet,
        )
        self.assertIn(
            "set(VCPKG_LINKER_FLAGS "
            '"-pthread -fwasm-exceptions -sSUPPORT_LONGJMP=wasm")',
            triplet,
        )
        self.assertNotIn(
            "set(VCPKG_LIBRARY_LINKAGE dynamic)",
            triplet,
        )
        self.assertNotIn("if(PORT MATCHES", triplet)

    def test_target_triplet_passes_canonical_emsdk_to_vcpkg_builds(
        self,
    ) -> None:
        triplet = (
            REPO / "vcpkgTriplets" / "wasm32-emscripten-rg.cmake"
        ).read_text("utf-8")
        passthrough = re.search(
            r"set\(VCPKG_ENV_PASSTHROUGH(?P<body>.*?)\)",
            triplet,
            re.DOTALL,
        )
        self.assertIsNotNone(passthrough)
        variables = passthrough.group("body").split()
        self.assertIn("EMSDK", variables)
        self.assertIn("EMSDK_PYTHON", variables)
        self.assertEqual(variables.count("EMSDK"), 1)

    def test_host_triplet_passes_reproducible_environment_to_vcpkg_builds(
        self,
    ) -> None:
        triplet = (
            REPO / "vcpkgTriplets" / "x64-windows-rg-host-release.cmake"
        ).read_text("utf-8")
        passthrough = re.search(
            r"set\(VCPKG_ENV_PASSTHROUGH(?P<body>.*?)\)",
            triplet,
            re.DOTALL,
        )
        self.assertIsNotNone(passthrough)
        variables = passthrough.group("body").split()
        self.assertEqual(
            variables,
            ["PYTHONNOUSERSITE", "SOURCE_DATE_EPOCH"],
        )

    def test_wasm_wrapper_forwards_all_vcpkg_flags(self) -> None:
        wrapper = (
            REPO / "cmake" / "toolchains" / "vcpkg-emscripten.cmake"
        ).read_text("utf-8")
        for variable in (
            "VCPKG_C_FLAGS",
            "VCPKG_CXX_FLAGS",
            "VCPKG_C_FLAGS_DEBUG",
            "VCPKG_CXX_FLAGS_DEBUG",
            "VCPKG_C_FLAGS_RELEASE",
            "VCPKG_CXX_FLAGS_RELEASE",
            "VCPKG_LINKER_FLAGS",
            "VCPKG_LINKER_FLAGS_DEBUG",
            "VCPKG_LINKER_FLAGS_RELEASE",
        ):
            self.assertIn(variable, wrapper)
        self.assertIn("Emscripten.cmake", wrapper)

    def test_wasm_wrapper_forces_ninja_response_files(self) -> None:
        chainload = (
            REPO / "cmake" / "toolchains" / "vcpkg-emscripten.cmake"
        ).read_text("utf-8")
        invocation_wrapper = (
            PROBE / "scripts" / "Invoke-WithToolchains.ps1"
        ).read_text("utf-8")
        triplet = (
            REPO / "vcpkgTriplets" / "wasm32-emscripten-rg.cmake"
        ).read_text("utf-8")
        self.assertIn(
            "$env:CMAKE_NINJA_FORCE_RESPONSE_FILE = '1'",
            invocation_wrapper,
        )
        self.assertIn(
            "CMAKE_NINJA_FORCE_RESPONSE_FILE",
            triplet,
        )
        self.assertNotIn(
            "set(CMAKE_C_COMPILER",
            chainload,
        )

    def test_wrapper_scrubs_ambient_flags_and_authenticates_before_execution(
        self,
    ) -> None:
        wrapper = (
            PROBE / "scripts" / "Invoke-WithToolchains.ps1"
        ).read_text("utf-8")
        bootstrap = (
            PROBE / "scripts" / "Bootstrap-Toolchains.ps1"
        ).read_text("utf-8")
        provenance = (
            PROBE / "scripts" / "Toolchain-Provenance.ps1"
        ).read_text("utf-8")
        chainload = (
            REPO / "cmake" / "toolchains" / "vcpkg-emscripten.cmake"
        ).read_text("utf-8")
        for variable in (
            "EMCC_CFLAGS",
            "CFLAGS",
            "CXXFLAGS",
            "CPPFLAGS",
            "LDFLAGS",
            "EM_CONFIG",
            "EM_CACHE",
            "EM_PORTS",
            "EM_COMPILER_WRAPPER",
            "EM_COMPILER_WRAPPER2",
            "BASH_ENV",
            "ENV",
            "QML_",
            "QT_",
            "SOURCE_DATE_EPOCH",
            "QT_RCC_SOURCE_DATE_OVERRIDE",
            "VCPKG_MAX_CONCURRENCY",
            "RHYTHMGAME_",
            "_EMCC_CCACHE",
        ):
            self.assertIn(variable, provenance)
        self.assertIn(
            "$env:SOURCE_DATE_EPOCH = $parsedSourceDateEpoch.ToString(",
            wrapper,
        )
        self.assertIn(
            "$env:VCPKG_MAX_CONCURRENCY = "
            "$parsedVcpkgMaxConcurrency.ToString(",
            wrapper,
        )
        for script in (wrapper, bootstrap):
            self.assertIn("Clear-WasmBuildEnvironment", script)
            self.assertIn("Assert-SourceArchiveInstallation", script)
            self.assertIn("Assert-FileSha256", script)
            self.assertIn("Assert-EmscriptenInstallation", script)
            self.assertIn("Assert-BuildToolInstallation", script)
        self.assertNotIn("-fexceptions", (
            REPO / "vcpkgTriplets" / "wasm32-emscripten-rg.cmake"
        ).read_text("utf-8"))
        self.assertNotIn(
            "set(CMAKE_CXX_COMPILER",
            chainload,
        )

    def test_qtbase_overlay_enables_features_and_keeps_version_check(self) -> None:
        overlay = REPO / "vcpkgOverlayPortsWasm" / "qtbase"
        portfile = (overlay / "portfile.cmake").read_text("utf-8")
        manifest = json.loads((overlay / "vcpkg.json").read_text("utf-8"))
        self.assertEqual(manifest["port-version"], 4)
        self.assertIn("restore-wasm-version-check.patch", portfile)
        self.assertIn(
            "preserve-wasm-event-composed-path.patch",
            portfile,
        )
        self.assertIn("-DFEATURE_thread:BOOL=ON", portfile)
        self.assertIn("-DFEATURE_wasm_exceptions:BOOL=ON", portfile)
        self.assertIn("-DFEATURE_wasm_jspi:BOOL=ON", portfile)
        self.assertIn("-DFEATURE_wasm_simd128:BOOL=OFF", portfile)
        self.assertNotIn(
            '"_qt_test_emscripten_version()" ""',
            portfile,
        )
        patch = (overlay / "restore-wasm-version-check.patch").read_text(
            "utf-8"
        )
        attributes = (REPO / ".gitattributes").read_text("utf-8")
        self.assertIn(
            "vcpkgOverlayPortsWasm/qtbase/** -text -whitespace",
            attributes,
        )
        self.assertIn("include(QtPublicWasmToolchainHelpers)", patch)
        self.assertIn(
            (
                '"${QT6_INSTALL_PREFIX}/${QT6_INSTALL_HEADERS}/'
                'QtCore/qconfig.h"'
            ),
            patch,
        )
        self.assertIn(
            '"${WASM_BUILD_DIR}/src/corelib/global/qconfig.h"',
            patch,
        )
        self.assertIn(
            '"${WASM_BUILD_DIR}/include/QtCore/qconfig.h"',
            patch,
        )
        self.assertEqual(
            hashlib.sha256(
                (overlay / "restore-wasm-version-check.patch").read_bytes()
            ).hexdigest(),
            QTBASE_WASM_PATCH_SHA256,
        )
        event_path_patch = (
            overlay / "preserve-wasm-event-composed-path.patch"
        )
        event_path_text = event_path_patch.read_text("utf-8")
        for marker in (
            "const composedPath = Object.freeze("
            "Array.from(obj.composedPath()));",
            "objCopy['composedPath'] = () => composedPath;",
            "Event.composedPath() becomes empty after native dispatch",
            "bool qtSendPendingEvents()",
            "bool qtSendPendingApplicationEvents()",
            'control["exclusiveEventHandler"].as<int>() > 0',
            "return false;",
            "This Embind entry intentionally stays synchronous",
            "attempts to suspend through this boundary traps the runtime",
            "#include <private/qeventdispatcher_wasm_p.h>",
            "static bool sendPendingNativeEventsFromBrowser();",
            "static bool sendPendingApplicationEventsFromBrowser();",
            "static thread_local int eventDeliveryDepth = 0;",
            "QEventDispatcherWasm::sendPendingNativeEventsFromBrowser()",
            "QEventDispatcherWasm::sendPendingApplicationEventsFromBrowser()",
            "#include <QtCore/qscopeguard.h>",
            "const auto eventDeliveryDepthGuard = qScopeGuard",
            "eventDeliveryDepth -= deliveryDepthIncrement",
            "eventDeliveryDepth > 0",
            "isValidEventDispatcherPointer(eventDispatcher)",
            "eventDispatcher->sendPostedEvents();",
            "suspendResumeControl->sendPendingEvents();",
            "eventDispatcher->sendTimerEvents();",
            (
                "QCoreApplication::sendPostedEvents("
                "nullptr, QEvent::DeferredDelete);"
            ),
            "QEventDispatcherWasm::sendPendingNativeEventsFromBrowser();",
            "sendPendingApplicationEventsFromBrowser();",
            "std::list<emscripten::val> m_currentEvents",
            "m_currentEvents.erase(currentEventContext)",
            "it->second(*currentEventContext)",
        ):
            with self.subTest(event_path_marker=marker):
                self.assertIn(marker, event_path_text)
        added_lines = "\n".join(
            line[1:]
            for line in event_path_text.splitlines()
            if line.startswith("+") and not line.startswith("+++")
        )
        patched_view = "\n".join(
            line[1:]
            for line in event_path_text.splitlines()
            if (
                line.startswith(("+", " "))
                and not line.startswith("+++")
            )
        )
        self.assertNotIn(
            "friend bool qtSendPendingEvents();",
            patched_view,
        )
        self.assertNotIn(
            "setCurrentEvent(emscripten::val::undefined())",
            patched_view,
        )
        self.assertNotIn(
            "QScopedValueRollback currentEventRollback",
            patched_view,
        )
        self.assertNotIn(
            "QScopedValueRollback eventDeliveryDepthRollback",
            patched_view,
        )

        send_all = re.search(
            (
                r"bool QEventDispatcherWasm::sendAllEvents"
                r"\(QEventLoop::ProcessEventsFlags flags\)\s*"
                r"\{(?P<body>.*?)"
                r"\n\}"
            ),
            patched_view,
            re.DOTALL,
        )
        self.assertIsNotNone(send_all)
        self.assertIn(
            (
                "++eventDeliveryDepth;\n"
                "    const auto eventDeliveryDepthGuard = qScopeGuard([] {\n"
                "        Q_ASSERT(eventDeliveryDepth > 0);\n"
                "        --eventDeliveryDepth;"
            ),
            send_all.group("body"),
        )

        native_cycle = re.search(
            (
                r"bool QEventDispatcherWasm::"
                r"sendPendingNativeEventsFromBrowser\(\)\s*"
                r"\{(?P<body>.*?)"
                r"\n\}"
            ),
            added_lines,
            re.DOTALL,
        )
        self.assertIsNotNone(native_cycle)
        native_cycle_body = native_cycle.group("body")
        native_main_guard = native_cycle_body.index(
            "if (!emscripten_is_main_runtime_thread())",
        )
        native_control_lookup = native_cycle_body.index(
            "g_mainThreadSuspendResumeControl",
        )
        native_exclusive_guard = native_cycle_body.index(
            'control["exclusiveEventHandler"].as<int>() > 0',
        )
        native_drain = native_cycle_body.index(
            "suspendResumeControl->sendPendingEvents();",
        )
        self.assertLess(native_main_guard, native_control_lookup)
        self.assertLess(native_exclusive_guard, native_drain)
        self.assertIn(
            "const int deliveryDepthIncrement = useAsyncify() ? 1 : 0",
            native_cycle_body,
        )
        native_increment = native_cycle_body.index(
            "eventDeliveryDepth += deliveryDepthIncrement",
        )
        native_decrement = native_cycle_body.index(
            "eventDeliveryDepth -= deliveryDepthIncrement",
        )
        self.assertLess(native_increment, native_decrement)
        self.assertLess(native_decrement, native_drain)
        self.assertEqual(
            native_cycle_body.count(
                "suspendResumeControl->sendPendingEvents();",
            ),
            1,
        )
        for forbidden in (
            "sendPostedEvents(",
            "sendTimerEvents(",
            "DeferredDelete",
            "processEvents(",
        ):
            with self.subTest(native_cycle_forbidden=forbidden):
                self.assertNotIn(forbidden, native_cycle_body)

        application_cycle = re.search(
            (
                r"bool QEventDispatcherWasm::"
                r"sendPendingApplicationEventsFromBrowser\(\)\s*"
                r"\{"
                r"(?P<body>.*?)"
                r"\n\}"
            ),
            added_lines,
            re.DOTALL,
        )
        self.assertIsNotNone(application_cycle)
        application_cycle_body = application_cycle.group("body")
        application_main_guard = application_cycle_body.index(
            "if (!emscripten_is_main_runtime_thread())",
        )
        application_dispatcher_lookup = application_cycle_body.index(
            "g_mainThreadEventDispatcher",
        )
        application_exclusive_guard = application_cycle_body.index(
            'control["exclusiveEventHandler"].as<int>() > 0',
        )
        application_depth_guard = application_cycle_body.index(
            "eventDeliveryDepth > 0",
        )
        depth_guard = application_cycle_body.index(
            "const auto eventDeliveryDepthGuard = qScopeGuard",
        )
        posted = application_cycle_body.index(
            "eventDispatcher->sendPostedEvents();",
        )
        native = application_cycle_body.index(
            "suspendResumeControl->sendPendingEvents();",
        )
        timers = application_cycle_body.index(
            "eventDispatcher->sendTimerEvents();",
        )
        deferred_delete = application_cycle_body.index(
            (
                "QCoreApplication::sendPostedEvents("
                "nullptr, QEvent::DeferredDelete);"
            ),
        )
        self.assertLess(application_main_guard, application_dispatcher_lookup)
        self.assertLess(application_exclusive_guard, depth_guard)
        self.assertLess(application_depth_guard, depth_guard)
        self.assertLess(depth_guard, posted)
        self.assertLess(posted, native)
        self.assertLess(native, timers)
        self.assertLess(timers, deferred_delete)
        self.assertEqual(
            application_cycle_body.count(
                "isValidEventDispatcherPointer(eventDispatcher)",
            ),
            4,
        )
        self.assertNotIn("processEvents(", application_cycle_body)

        self.assertIn(
            (
                "if (g_mainThreadEventDispatcher->m_isSendingNativeEvents\n"
                "        || eventDeliveryDepth > 0) {"
            ),
            added_lines,
        )
        self.assertEqual(
            patched_view.count(
                "QT_WASM_EMSCRIPTEN_ASYNC",
            ),
            1,
        )
        self.assertIn(
            (
                'emscripten::function("qtSendPendingEvents", '
                "qtSendPendingEvents);"
            ),
            patched_view,
        )
        self.assertIn(
            (
                'emscripten::function("qtSendPendingApplicationEvents",\n'
                "                         qtSendPendingApplicationEvents "
                "QT_WASM_EMSCRIPTEN_ASYNC);"
            ),
            patched_view,
        )
        self.assertEqual(
            hashlib.sha256(event_path_patch.read_bytes()).hexdigest(),
            QTBASE_WASM_EVENT_PATH_PATCH_SHA256,
        )
        emscripten_patch_block = re.search(
            (
                r"if\(VCPKG_TARGET_IS_EMSCRIPTEN\)\s*"
                r"list\(APPEND \$\{PORT\}_PATCHES "
                r"preserve-wasm-event-composed-path\.patch\)\s*"
                r"endif\(\)"
            ),
            portfile,
        )
        self.assertIsNotNone(emscripten_patch_block)

    def test_qtmultimedia_overlay_defers_wasm_device_notifications(
        self,
    ) -> None:
        overlay = REPO / "vcpkgOverlayPortsWasm" / "qtmultimedia"
        portfile = (overlay / "portfile.cmake").read_text("utf-8")
        manifest = json.loads((overlay / "vcpkg.json").read_text("utf-8"))
        port_data = (overlay / "port.data.cmake").read_text("utf-8")
        patch_path = (
            overlay / "defer-wasm-media-device-notifications.patch"
        )
        patch_text = patch_path.read_text("utf-8")
        path_map_patch = (
            overlay / "canonicalize-wasm-build-paths.patch"
        )
        path_map_text = path_map_patch.read_text("utf-8")
        lifecycle_patch = (
            overlay / "correct-wasm-media-lifecycle.patch"
        )
        lifecycle_patch_text = lifecycle_patch.read_text("utf-8")

        self.assertEqual(manifest["version"], "6.11.1")
        self.assertEqual(manifest["port-version"], 2)
        self.assertIn(QTMULTIMEDIA_VCPKG_SHA512, port_data)
        self.assertRegex(
            portfile,
            (
                r"if\(VCPKG_TARGET_IS_EMSCRIPTEN\)\s*"
                r"(?s:.*?)"
                r"list\(APPEND \$\{PORT\}_PATCHES\s*"
                r"defer-wasm-media-device-notifications\.patch\s*"
                r"correct-wasm-media-lifecycle\.patch\s*"
                r"canonicalize-wasm-build-paths\.patch\s*\)\s*"
                r"endif\(\)"
            ),
        )
        self.assertEqual(
            hashlib.sha256(patch_path.read_bytes()).hexdigest(),
            QTMULTIMEDIA_WASM_PATCH_SHA256,
        )
        self.assertEqual(
            hashlib.sha256(path_map_patch.read_bytes()).hexdigest(),
            QTMULTIMEDIA_WASM_PATH_MAP_PATCH_SHA256,
        )
        self.assertEqual(
            hashlib.sha256(lifecycle_patch.read_bytes()).hexdigest(),
            QTMULTIMEDIA_WASM_MEDIA_LIFECYCLE_PATCH_SHA256,
        )
        for fragment in (
            "if(EMSCRIPTEN)",
            "-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}"
            "=/qt/qtmultimedia/source",
            "-fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}"
            "=/qt/qtmultimedia/source",
            "-ffile-prefix-map=${CMAKE_CURRENT_BINARY_DIR}"
            "=/qt/qtmultimedia/build",
            "-fmacro-prefix-map=${CMAKE_CURRENT_BINARY_DIR}"
            "=/qt/qtmultimedia/build",
        ):
            self.assertEqual(path_map_text.count(fragment), 1)
        for commit in QTMULTIMEDIA_WASM_UPSTREAM_COMMITS[:-3]:
            with self.subTest(upstream_commit=commit):
                self.assertIn(commit, patch_text)
        for commit in QTMULTIMEDIA_WASM_UPSTREAM_COMMITS[-3:]:
            with self.subTest(lifecycle_upstream_commit=commit):
                self.assertIn(commit, lifecycle_patch_text)

        lifecycle_removed_lines = "\n".join(
            line[1:]
            for line in lifecycle_patch_text.splitlines()
            if line.startswith("-") and not line.startswith("---")
        )
        lifecycle_patched_view = "\n".join(
            line[1:]
            for line in lifecycle_patch_text.splitlines()
            if (
                line.startswith(("+", " "))
                and not line.startswith("+++")
            )
        )
        ended_start = lifecycle_patched_view.index("auto endedCallback")
        ended_end = lifecycle_patched_view.index(
            "m_endedEvent.reset",
            ended_start,
        )
        ended_view = lifecycle_patched_view[ended_start:ended_end]
        emptied_start = lifecycle_patched_view.index(
            "auto emptiedCallback"
        )
        emptied_end = lifecycle_patched_view.index(
            "m_emptiedChangeEvent.reset",
            emptied_start,
        )
        emptied_view = lifecycle_patched_view[
            emptied_start:emptied_end
        ]
        self.assertIn("MediaStatus::EndOfMedia", ended_view)
        self.assertIn("emit statusChanged", ended_view)
        self.assertNotIn("stop();", ended_view)
        self.assertNotIn("m_shouldStop", ended_view)
        self.assertIn("emit readyChanged(false);", emptied_view)
        self.assertNotIn(
            "m_currentMediaStatus = MediaStatus::EndOfMedia;",
            emptied_view,
        )
        self.assertEqual(
            lifecycle_removed_lines.count(
                "m_currentMediaStatus = MediaStatus::EndOfMedia;"
            ),
            1,
        )
        self.assertIn(
            "qWarning() << Q_FUNC_INFO << __LINE__;",
            lifecycle_removed_lines,
        )
        self.assertNotIn(
            "qWarning() << Q_FUNC_INFO << __LINE__;",
            lifecycle_patched_view,
        )
        stop_start = lifecycle_patched_view.index(
            "void QWasmVideoOutput::stop()"
        )
        stop_end = lifecycle_patched_view.index(
            "void QWasmVideoOutput::pause()",
            stop_start,
        )
        stop_view = lifecycle_patched_view[stop_start:stop_end]
        self.assertLess(
            stop_view.index("if (m_shouldStop)"),
            stop_view.index("qCDebug"),
        )
        self.assertIn(
            "m_mediaInputStream && m_mediaInputStream->isActive()",
            stop_view,
        )
        self.assertIn(
            "JsMediaInputStream *m_mediaInputStream = nullptr;",
            lifecycle_patched_view,
        )
        destructor_start = lifecycle_patched_view.index(
            "QWasmVideoOutput::~QWasmVideoOutput()"
        )
        destructor_end = lifecycle_patched_view.index(
            "void QWasmVideoOutput::setVideoSize",
            destructor_start,
        )
        destructor_view = lifecycle_patched_view[
            destructor_start:destructor_end
        ]
        terminal_latch = destructor_view.index("m_shouldStop = true;")
        source_clear = destructor_view.index('removeAttribute", std::string("src")')
        source_object_clear = destructor_view.index(
            'm_video.set("srcObject", emscripten::val::null())'
        )
        load = destructor_view.index('m_video.call<void>("load")')
        remove = destructor_view.index('m_video.call<void>("remove")')
        self.assertLess(terminal_latch, source_clear)
        self.assertLess(source_clear, source_object_clear)
        self.assertLess(source_object_clear, load)
        self.assertLess(load, remove)
        self.assertEqual(
            destructor_view.count(
                "m_currentVideoMode == QWasmVideoOutput::VideoDisplay"
            ),
            2,
        )
        self.assertIn(
            "!srcObject.isUndefined() && !srcObject.isNull()",
            destructor_view,
        )
        self.assertNotIn("stop();", destructor_view)
        self.assertNotIn("stopMediaStream", destructor_view)
        player_destructor_start = lifecycle_patched_view.index(
            "QWasmMediaPlayer::~QWasmMediaPlayer()"
        )
        player_destructor_view = lifecycle_patched_view[
            player_destructor_start:
        ]
        self.assertLess(
            player_destructor_view.index(
                "m_videoOutput->disconnect(this);"
            ),
            player_destructor_view.index("delete m_videoOutput;"),
        )

        patch_lines = patch_text.splitlines()
        parsed_hunks = 0
        line_index = 0
        while line_index < len(patch_lines):
            hunk_header = re.fullmatch(
                (
                    r"@@ -\d+(?:,(?P<old_count>\d+))? "
                    r"\+\d+(?:,(?P<new_count>\d+))? @@.*"
                ),
                patch_lines[line_index],
            )
            if hunk_header is None:
                line_index += 1
                continue
            parsed_hunks += 1
            expected_old = int(hunk_header.group("old_count") or "1")
            expected_new = int(hunk_header.group("new_count") or "1")
            actual_old = 0
            actual_new = 0
            line_index += 1
            while (
                line_index < len(patch_lines)
                and not patch_lines[line_index].startswith(("@@ ", "diff --git "))
            ):
                prefix = patch_lines[line_index][:1]
                actual_old += prefix in (" ", "-")
                actual_new += prefix in (" ", "+")
                line_index += 1
            self.assertEqual(actual_old, expected_old)
            self.assertEqual(actual_new, expected_new)
        self.assertGreater(parsed_hunks, 0)

        added_lines = "\n".join(
            line[1:]
            for line in patch_text.splitlines()
            if line.startswith("+") and not line.startswith("+++")
        )
        removed_lines = "\n".join(
            line[1:]
            for line in patch_text.splitlines()
            if line.startswith("-") and not line.startswith("---")
        )
        patched_view = "\n".join(
            line[1:]
            for line in patch_text.splitlines()
            if (
                line.startswith(("+", " "))
                and not line.startswith("+++")
            )
        )
        get_media_start = patched_view.index(
            "void QWasmMediaDevices::getMediaDevices()"
        )
        get_media_end = patched_view.index(
            "auto capture = alcGetString(",
            get_media_start,
        )
        get_media_view = patched_view[get_media_start:get_media_end]
        self.assertNotIn("qstdweb::haveAsyncify()", get_media_view)
        self.assertNotIn(".await()", get_media_view)
        self.assertNotIn("m_devicesList", patched_view)
        self.assertEqual(
            get_media_view.count(
                "qstdweb::PromiseCallbacks enumerateDevicesCallbacks"
            ),
            1,
        )
        self.assertEqual(
            get_media_view.count("std::move(enumerateDevicesCallbacks)"),
            1,
        )
        self.assertEqual(
            get_media_view.count("requestMediaDevicesEnumeration();"),
            3,
        )
        request_start = get_media_view.index(
            "void QWasmMediaDevices::requestMediaDevicesEnumeration()"
        )
        finish_start = get_media_view.index(
            "void QWasmMediaDevices::finishMediaDevicesEnumeration()"
        )
        callbacks_start = get_media_view.index(
            "qstdweb::PromiseCallbacks enumerateDevicesCallbacks",
            request_start,
        )
        promise_start = get_media_view.index(
            "qstdweb::Promise::make(",
            callbacks_start,
        )
        event_start = get_media_view.index(
            "m_deviceChangedCallback",
        )
        self.assertLess(event_start, request_start)
        self.assertLess(request_start, callbacks_start)
        self.assertLess(callbacks_start, promise_start)
        self.assertLess(promise_start, finish_start)
        self.assertIn(
            "[this](emscripten::val)",
            get_media_view[event_start:request_start],
        )
        self.assertIn(
            "if (m_mediaDevicesEnumerationInFlight)",
            get_media_view[request_start:callbacks_start],
        )
        self.assertIn(
            "m_mediaDevicesEnumerationDirty = true;",
            get_media_view[request_start:callbacks_start],
        )
        self.assertEqual(
            get_media_view.count("finishMediaDevicesEnumeration();"),
            2,
        )
        self.assertIn(
            "if (!m_mediaDevicesEnumerationDirty)",
            get_media_view[finish_start:],
        )
        self.assertIn(
            "m_mediaDevicesEnumerationDirty = false;",
            get_media_view[finish_start:],
        )
        self.assertEqual(
            added_lines.count("QMetaObject::invokeMethod("),
            5,
        )
        self.assertEqual(added_lines.count("Qt::QueuedConnection);"), 5)
        for synchronous_call in (
            "videoDevices->onVideoInputsChanged();",
            "audioDevices->onAudioInputsChanged();",
            "audioDevices->onAudioOutputsChanged();",
        ):
            with self.subTest(synchronous_call=synchronous_call):
                self.assertNotIn(synchronous_call, patched_view)
        self.assertNotIn(
            'qWarning() << "m_audioInputs count"',
            patched_view,
        )
        self.assertIn(
            (
                "if (capture && "
                "!m_audioInputs.contains(m_openALAudioInputId))"
            ),
            added_lines,
        )
        self.assertIn(
            (
                "if (playback && "
                "!m_audioOutputs.contains(m_openALAudioOutputId))"
            ),
            added_lines,
        )
        self.assertIn(
            'm_jsMediaDevicesInterface, "devicechange",',
            added_lines,
        )
        self.assertNotIn("asyncEnumerate", patched_view)
        for marker in (
            "audioOutputsToRemove = m_audioOutputs.keys();",
            "audioInputsToRemove = m_audioInputs.keys();",
            "audioOutputsToRemove.removeOne(m_openALAudioOutputId);",
            "audioInputsToRemove.removeOne(m_openALAudioInputId);",
            "const auto hasDefaultDevice",
            "isDefault = !audioInputDefaultAssigned;",
            "isDefault = !audioOutputDefaultAssigned;",
            "m_audioOutputsAdded = true;",
            "std::string m_openALAudioInputId;",
            "std::string m_openALAudioOutputId;",
            "void requestMediaDevicesEnumeration();",
            "void finishMediaDevicesEnumeration();",
            "bool m_mediaDevicesEnumerationInFlight = false;",
            "bool m_mediaDevicesEnumerationDirty = false;",
        ):
            with self.subTest(reconciliation_marker=marker):
                self.assertIn(marker, patched_view)
        self.assertNotIn("m_firstInit", patched_view)
        self.assertEqual(
            added_lines.count("getOpenALAudioDevices();"),
            0,
        )
        self.assertEqual(
            removed_lines.count("getOpenALAudioDevices();"),
            1,
        )
        removal_list_index = patched_view.index(
            "audioOutputsToRemove = m_audioOutputs.keys();"
        )
        fallback_exclusion_index = patched_view.index(
            "audioOutputsToRemove.removeOne(m_openALAudioOutputId);"
        )
        enumeration_index = patched_view.index(
            'deviceKind == std::string("audioinput")'
        )
        self.assertLess(removal_list_index, fallback_exclusion_index)
        self.assertLess(fallback_exclusion_index, enumeration_index)
        constructor = re.search(
            (
                r"QWasmMediaDevices::QWasmMediaDevices\(\)\s*"
                r"\{(?P<body>.*?)\n\}"
            ),
            patched_view,
            re.DOTALL,
        )
        self.assertIsNotNone(constructor)
        self.assertNotIn("initDevices()", constructor.group("body"))
        for singleton_marker in (
            "static QWasmMediaDevices *s_mediaDevicesInstance = nullptr;",
            "static bool s_constructingInstance = false;",
            "s_mediaDevicesInstance = new QWasmMediaDevices();",
            "s_mediaDevicesInstance->initDevices();",
        ):
            with self.subTest(singleton_marker=singleton_marker):
                self.assertIn(singleton_marker, patched_view)

    def test_qtbase_overlay_seeds_wasm_mkspec_before_project(self) -> None:
        portfile = (
            REPO / "vcpkgOverlayPortsWasm" / "qtbase" / "portfile.cmake"
        ).read_text("utf-8")
        option = "-DQT_QMAKE_TARGET_MKSPEC:STRING=wasm-emscripten"
        emscripten_options = re.search(
            (
                r"if\(VCPKG_TARGET_IS_EMSCRIPTEN\)\s*"
                r"list\(APPEND FEATURE_OPTIONS(?P<body>.*?)\)\s*"
                r"endif\(\)"
            ),
            portfile,
            re.DOTALL,
        )
        self.assertIsNotNone(emscripten_options)
        self.assertIn(option, emscripten_options.group("body"))
        self.assertEqual(portfile.count(option), 1)

    def test_installed_qt_rejects_mismatched_emscripten(self) -> None:
        cmake = (
            REPO
            / ".toolchains"
            / "cmake-4.2.3-windows-x86_64"
            / "bin"
            / "cmake.exe"
        )
        qt_prefix = (
            REPO
            / ".wasm-vcpkg"
            / "installed"
            / "wasm32-emscripten-rg"
        )
        qt_cmake = qt_prefix / "share" / "Qt6"
        qt_core_cmake = qt_prefix / "share" / "Qt6Core"
        required = (
            cmake,
            qt_cmake / "Qt6Config.cmake",
            qt_cmake / "QtInstallPaths.cmake",
            qt_cmake / "QtPublicWasmToolchainHelpers.cmake",
            qt_core_cmake / "Qt6CoreConfig.cmake",
            qt_core_cmake / "Qt6CoreConfigExtras.cmake",
        )
        if not all(path.is_file() for path in required):
            self.skipTest("requires the generated wasm-release Qt install")

        qt6_config = required[1].read_text("utf-8")
        install_paths = required[2].read_text("utf-8")
        core_config = required[4].read_text("utf-8")
        core_extras = required[5].read_text("utf-8")
        self.assertIn(
            'set(QT6_INSTALL_HEADERS "include/Qt6")',
            install_paths,
        )
        self.assertLess(
            qt6_config.index("QtInstallPaths.cmake"),
            qt6_config.index("find_package(Qt6${module}"),
        )
        self.assertIn("Qt6CoreConfigExtras.cmake", core_config)
        self.assertIn("Qt6WasmMacros.cmake", core_extras)

        with tempfile.TemporaryDirectory(
            prefix="rhythm-game-wasm-mismatch-"
        ) as temporary:
            temporary_path = Path(temporary)
            fake_sdk = temporary_path / "fake-emsdk"
            fake_emcc = (
                fake_sdk / "upstream" / "emscripten" / "emcc.bat"
            )
            fake_emcc.parent.mkdir(parents=True)
            (fake_sdk / ".emscripten").write_text(
                (
                    "emsdk_path = r'ignored-by-qt-regex'\n"
                    "EMSCRIPTEN_ROOT = "
                    "emsdk_path + '/upstream/emscripten'\n"
                ),
                encoding="utf-8",
            )
            fake_emcc.write_text(
                (
                    "@echo off\n"
                    "echo emcc (Emscripten compiler) 9.9.9\n"
                    "exit /b 0\n"
                ),
                encoding="utf-8",
            )
            check = temporary_path / "check-mismatch.cmake"
            check.write_text(
                (
                    f'include("{required[2].as_posix()}")\n'
                    "if(NOT QT6_INSTALL_HEADERS "
                    'STREQUAL "include/Qt6")\n'
                    '    message(FATAL_ERROR "missing install headers")\n'
                    "endif()\n"
                    f'include("{required[3].as_posix()}")\n'
                    "_qt_test_emscripten_version()\n"
                    'message(FATAL_ERROR "MISMATCH_SENTINEL")\n'
                ),
                encoding="utf-8",
            )
            environment = os.environ.copy()
            environment["EMSDK"] = str(fake_sdk)
            result = subprocess.run(
                [str(cmake), "-P", str(check)],
                cwd=REPO,
                env=environment,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

        output = result.stdout + result.stderr
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("MISMATCH_SENTINEL", output)
        self.assertIn(
            "Qt Wasm was built with Emscripten version: 4.0.7",
            output,
        )
        self.assertIn(
            "You are using Emscripten version: 9.9.9",
            output,
        )
        self.assertIn(
            "Stopping configuration due to mismatch",
            output,
        )

    def test_qtdeclarative_disables_long_path_styles_only_for_host_tools(
        self,
    ) -> None:
        portfile = (
            REPO / "vcpkgOverlayPorts" / "qtdeclarative" / "portfile.cmake"
        ).read_text("utf-8")
        fluent_option = "-DFEATURE_quickcontrols2_fluentwinui3:BOOL=OFF"
        universal_option = "-DFEATURE_quickcontrols2_universal:BOOL=OFF"
        self.assertEqual(portfile.count(fluent_option), 1)
        self.assertEqual(portfile.count(universal_option), 1)
        self.assertNotIn("CMAKE_OBJECT_PATH_MAX", portfile)
        self.assertRegex(
            portfile,
            (
                r"(?s)if\(VCPKG_TARGET_IS_WINDOWS AND\s+"
                r'TARGET_TRIPLET STREQUAL "'
                r'x64-windows-rg-host-release"\).*?'
                + fluent_option
                + r".*?"
                + universal_option
                + r".*?endif\(\)"
            ),
        )
        self.assertNotIn("VCPKG_TARGET_TRIPLET STREQUAL", portfile)

    def test_wasm_autogen_uses_batch_safe_response_threshold(self) -> None:
        portfile = (
            REPO / "vcpkgOverlayPorts" / "qtdeclarative" / "portfile.cmake"
        ).read_text("utf-8")
        option = (
            "-DCMAKE_AUTOGEN_COMMAND_LINE_LENGTH_MAX:STRING=4096"
        )
        self.assertIn(option, portfile)
        self.assertRegex(
            portfile,
            (
                r"(?s)if\(VCPKG_CMAKE_SYSTEM_NAME STREQUAL "
                r'"Emscripten"\).*?'
                + option
                + r".*?endif\(\)"
            ),
        )
        presets = json.loads((PROBE / "CMakePresets.json").read_text("utf-8"))
        cache = presets["configurePresets"][0]["cacheVariables"]
        self.assertEqual(
            cache["CMAKE_AUTOGEN_COMMAND_LINE_LENGTH_MAX"],
            "4096",
        )
        self.assertNotIn("CMAKE_AUTOMOC_COMPILER_PREDEFINES", portfile)
        self.assertNotIn(
            "CMAKE_AUTOMOC_COMPILER_PREDEFINES",
            json.dumps(presets),
        )

    def test_existing_qtdeclarative_patch_bytes_are_unchanged(self) -> None:
        patch = (
            REPO
            / "vcpkgOverlayPorts"
            / "qtdeclarative"
            / "24205cd-qquickwindow-child-window-stacking.patch"
        )
        digest = hashlib.sha256(patch.read_bytes()).hexdigest().upper()
        self.assertEqual(digest, QTDECLARATIVE_PATCH_SHA256)


if __name__ == "__main__":
    unittest.main()
