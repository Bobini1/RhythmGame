#include "dxa.h"

#include "PathToUtfString.h"
#include "UtfStringToPath.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include <spdlog/spdlog.h>

// Taken from Lunatic Vibes

// Codes are from DXArchive (DX Library -> Tool -> DXArchive -> Source) , with
// some modification Original author: 山田 巧 (Takumi Yamada) Homepage:
// https://dxlib.xsrv.jp/dxtec.html

namespace dxa {

#define FILE_ATTRIBUTE_DIRECTORY 0x00000010

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

const u32 MIN_COMPRESS_VER5 = 4;

const u16 DXA_HEAD_VER5 = 0x5844;        // "DX"
const u16 DXA_VER_VER5 = (0x0005);       // バージョン
const u32 DXA_KEYSTR_LENGTH_VER5 = (12); // 鍵文字列の長さ

#pragma pack(push)
#pragma pack(1)

// アーカイブデータの最初のヘッダ
using DARC_HEAD_VER5 = struct tagDARC_HEAD_VER5
{
    u16 Head;     // ヘッダ
    u16 Version;  // バージョン
    u32 HeadSize; // ヘッダ情報の DARC_HEAD_VER5 を抜いた全サイズ
    u32
      DataStartAddress; // 最初のファイルのデータが格納されているデータアドレス(ファイルの先頭アドレスをアドレス０とする)
    u32
      FileNameTableStartAddress; // ファイル名テーブルの先頭アドレス(ファイルの先頭アドレスをアドレス０とする)
    u32
      FileTableStartAddress; // ファイルテーブルの先頭アドレス(メンバ変数
                             // FileNameTableStartAddress のアドレスを０とする)
    u32
      DirectoryTableStartAddress; // ディレクトリテーブルの先頭アドレス(メンバ変数
                                  // FileNameTableStartAddress
                                  // のアドレスを０とする)
                                  // アドレス０から配置されている
                                  // DARC_DIRECTORY_VER5
                                  // 構造体がルートディレクトリ
    u32 CodePage; // ファイル名に使用しているコードページ番号
};

// アーカイブデータの最初のヘッダ(Ver 0x0003まで)
using DARC_HEAD_VER3 = struct tagDARC_HEAD_VER3
{
    u16 Head;     // ヘッダ
    u16 Version;  // バージョン
    u32 HeadSize; // ヘッダ情報の DARC_HEAD_VER5 を抜いた全サイズ
    u32
      DataStartAddress; // 最初のファイルのデータが格納されているデータアドレス(ファイルの先頭アドレスをアドレス０とする)
    u32
      FileNameTableStartAddress; // ファイル名テーブルの先頭アドレス(ファイルの先頭アドレスをアドレス０とする)
    u32
      FileTableStartAddress; // ファイルテーブルの先頭アドレス(メンバ変数
                             // FileNameTableStartAddress のアドレスを０とする)
    u32
      DirectoryTableStartAddress; // ディレクトリテーブルの先頭アドレス(メンバ変数
                                  // FileNameTableStartAddress
                                  // のアドレスを０とする)
                                  // アドレス０から配置されている
                                  // DARC_DIRECTORY_VER5
                                  // 構造体がルートディレクトリ
};

// ファイルの時間情報
using DARC_FILETIME_VER5 = struct tagDARC_FILETIME_VER5
{
    u64 Create;     // 作成時間
    u64 LastAccess; // 最終アクセス時間
    u64 LastWrite;  // 最終更新時間
};

// ファイル格納情報(Ver 0x0001)
using DARC_FILEHEAD_VER1 = struct tagDARC_FILEHEAD_VER1
{
    u32 NameAddress; // ファイル名が格納されているアドレス( ARCHIVE_HEAD構造体
                     // のメンバ変数 FileNameTableStartAddress
                     // のアドレスをアドレス０とする)

    u32 Attributes;          // ファイル属性
    DARC_FILETIME_VER5 Time; // 時間情報
    u32 DataAddress;         // ファイルが格納されているアドレス
                             //			ファイルの場合：DARC_HEAD_VER5構造体
                             // のメンバ変数 DataStartAddress
    // が示すアドレスをアドレス０とする
    // ディレクトリの場合：DARC_HEAD_VER5構造体 のメンバ変数
    // DirectoryTableStartAddress のが示すアドレスをアドレス０とする
    u32 DataSize; // ファイルのデータサイズ
};

// ファイル格納情報
using DARC_FILEHEAD_VER5 = struct tagDARC_FILEHEAD_VER5
{
    u32 NameAddress; // ファイル名が格納されているアドレス( ARCHIVE_HEAD構造体
                     // のメンバ変数 FileNameTableStartAddress
                     // のアドレスをアドレス０とする)

    u32 Attributes;          // ファイル属性
    DARC_FILETIME_VER5 Time; // 時間情報
    u32 DataAddress;         // ファイルが格納されているアドレス
                             //			ファイルの場合：DARC_HEAD_VER5構造体
                             // のメンバ変数 DataStartAddress
    // が示すアドレスをアドレス０とする
    // ディレクトリの場合：DARC_HEAD_VER5構造体 のメンバ変数
    // DirectoryTableStartAddress のが示すアドレスをアドレス０とする
    u32 DataSize; // ファイルのデータサイズ
    u32
      CompressedDataSize; // 圧縮後のデータのサイズ( 0xffffffff:圧縮されていない
                          // ) ( Ver0x0002 で追加された )
};

// ディレクトリ格納情報
using DARC_DIRECTORY_VER5 = struct tagDARC_DIRECTORY_VER5
{
    u32 DirectoryAddress; // 自分の DARC_FILEHEAD_VER5 が格納されているアドレス(
                          // DARC_HEAD_VER5 構造体 のメンバ変数
                          // FileTableStartAddress
                          // が示すアドレスをアドレス０とする)
    u32
      ParentDirectoryAddress; // 親ディレクトリの DARC_DIRECTORY_VER5
                              // が格納されているアドレス( DARC_HEAD_VER5構造体
                              // のメンバ変数 DirectoryTableStartAddress
                              // が示すアドレスをアドレス０とする)
    u32 FileHeadNum;          // ディレクトリ内のファイルの数
    u32
      FileHeadAddress; // ディレクトリ内のファイルのヘッダ列が格納されているアドレス(
                       // DARC_HEAD_VER5構造体 のメンバ変数
                       // FileTableStartAddress
                       // が示すアドレスをアドレス０とする)
};

#pragma pack(pop)

// 鍵文字列を作成
static void
KeyCreate(const char* Source, unsigned char* Key)
{
    size_t Len;

    if (Source == nullptr) {
        memset(Key, 0xaaaaaaaa, DXA_KEYSTR_LENGTH_VER5);
    } else {
        Len = strlen(Source);
        if (Len > DXA_KEYSTR_LENGTH_VER5) {
            memcpy(Key, Source, DXA_KEYSTR_LENGTH_VER5);
        } else {
            // 鍵文字列が DXA_KEYSTR_LENGTH_VER5 より短かったらループする
            size_t i;

            for (i = 0; i + Len <= DXA_KEYSTR_LENGTH_VER5; i += Len)
                memcpy(Key + i, Source, Len);
            if (i < DXA_KEYSTR_LENGTH_VER5)
                memcpy(Key + i, Source, DXA_KEYSTR_LENGTH_VER5 - i);
        }
    }

    Key[0] = ~Key[0];
    Key[1] = (Key[1] >> 4) | (Key[1] << 4);
    Key[2] = Key[2] ^ 0x8a;
    Key[3] = ~((Key[3] >> 4) | (Key[3] << 4));
    Key[4] = ~Key[4];
    Key[5] = Key[5] ^ 0xac;
    Key[6] = ~Key[6];
    Key[7] = ~((Key[7] >> 3) | (Key[7] << 5));
    Key[8] = (Key[8] >> 5) | (Key[8] << 3);
    Key[9] = Key[9] ^ 0x7f;
    Key[10] = ((Key[10] >> 4) | (Key[10] << 4)) ^ 0xd6;
    Key[11] = Key[11] ^ 0xcc;
}

// 鍵文字列を使用して Xor 演算( Key は必ず DXA_KEYSTR_LENGTH_VER5
// の長さがなければならない )
static void
require(bool valid)
{
    if (!valid) {
        throw std::runtime_error("Invalid or truncated DXA archive");
    }
}

template<typename T>
auto
record(std::span<const u8> bytes, size_t offset) -> T
{
    require(offset <= bytes.size() && sizeof(T) <= bytes.size() - offset);
    T result;
    std::memcpy(&result, bytes.data() + offset, sizeof(T));
    return result;
}

static void
KeyConvFileRead(void* data,
                size_t size,
                std::ifstream& file,
                const unsigned char* key,
                std::optional<uint64_t> position = {})
{
    const auto start = file.tellg();
    require(start >= 0 &&
            size <= static_cast<uint64_t>(
                      std::numeric_limits<std::streamsize>::max()));
    if (size != 0) {
        file.read(static_cast<char*>(data), static_cast<std::streamsize>(size));
        require(file.good() && static_cast<size_t>(file.gcount()) == size);
    }
    auto keyIndex =
      position.value_or(static_cast<uint64_t>(start)) % DXA_KEYSTR_LENGTH_VER5;
    auto* bytes = static_cast<u8*>(data);
    for (size_t i = 0; i < size; ++i) {
        bytes[i] ^= key[keyIndex];
        keyIndex = (keyIndex + 1) % DXA_KEYSTR_LENGTH_VER5;
    }
}

// An empty destination validates the stream before allocating its output.
static void
Decompress(std::span<const u8> source,
           std::span<u8> destination,
           size_t expectedSize)
{
    require(source.size() >= 9);
    require(record<u32>(source, 0) == expectedSize);
    const auto encodedSize = record<u32>(source, 4);
    require(encodedSize >= 9 && encodedSize <= source.size());
    require(destination.empty() || destination.size() == expectedSize);
    const auto key = source[8];
    auto input = size_t{ 9 };
    auto output = size_t{};
    auto next = [&]() -> u8 {
        require(input < encodedSize);
        return source[input++];
    };
    auto literal = [&](u8 value) {
        require(output < expectedSize);
        if (!destination.empty())
            destination[output] = value;
        ++output;
    };
    while (input < encodedSize) {
        const auto value = next();
        if (value != key) {
            literal(value);
            continue;
        }
        auto code = next();
        if (code == key) {
            literal(key);
            continue;
        }
        if (code > key)
            --code;
        auto count = size_t(code >> 3);
        if (code & 4)
            count |= size_t(next()) << 5;
        count += MIN_COMPRESS_VER5;
        const auto indexSize = code & 3;
        require(indexSize < 3);
        auto distance = size_t{};
        for (int i = 0; i <= indexSize; ++i) {
            distance |= size_t(next()) << (8 * i);
        }
        ++distance;
        require(distance <= output && count <= expectedSize - output);
        if (!destination.empty()) {
            for (size_t i = 0; i < count; ++i) {
                destination[output + i] = destination[output + i - distance];
            }
        }
        output += count;
    }
    require(output == expectedSize);
}

static auto
DecodeArchive(const std::filesystem::path& path) -> support::DXArchive
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    require(file.is_open() && file.tellg() >= 0);
    const auto fileSize = static_cast<uint64_t>(file.tellg());
    auto range = [&](uint64_t offset, uint64_t size) {
        require(offset <= fileSize && size <= fileSize - offset);
    };
    range(0, sizeof(DARC_HEAD_VER3));
    auto key = std::array<u8, DXA_KEYSTR_LENGTH_VER5>{};
    KeyCreate(nullptr, key.data());
    auto header = DARC_HEAD_VER5{};
    file.seekg(0);
    KeyConvFileRead(&header, sizeof(DARC_HEAD_VER3), file, key.data(), 0);
    if (header.Head != DXA_HEAD_VER5) {
        key.fill(0xff);
        file.seekg(0);
        KeyConvFileRead(&header, sizeof(DARC_HEAD_VER3), file, key.data(), 0);
    }
    require(header.Head == DXA_HEAD_VER5 && header.Version >= 1 &&
            header.Version <= DXA_VER_VER5);
    const auto headerSize =
      header.Version <= 3 ? sizeof(DARC_HEAD_VER3) : sizeof(DARC_HEAD_VER5);
    range(0, headerSize);
    require(header.DataStartAddress >= headerSize);
    require(header.FileNameTableStartAddress >= headerSize);
    range(header.FileNameTableStartAddress, header.HeadSize);
    require(header.FileTableStartAddress <= header.DirectoryTableStartAddress &&
            header.DirectoryTableStartAddress <= header.HeadSize);
    auto tables = std::vector<u8>(header.HeadSize);
    file.seekg(header.FileNameTableStartAddress);
    KeyConvFileRead(tables.data(),
                    tables.size(),
                    file,
                    key.data(),
                    header.Version >= 5 ? std::optional<uint64_t>{ 0 }
                                        : std::nullopt);
    const auto bytes = std::span<const u8>(tables);
    const auto names = bytes.first(header.FileTableStartAddress);
    const auto files = bytes.subspan(header.FileTableStartAddress,
                                     header.DirectoryTableStartAddress -
                                       header.FileTableStartAddress);
    const auto directories = bytes.subspan(header.DirectoryTableStartAddress);
    const auto fileRecordSize = header.Version >= 2
                                  ? sizeof(DARC_FILEHEAD_VER5)
                                  : sizeof(DARC_FILEHEAD_VER1);
    auto fileRecord = [&](size_t offset) {
        require(offset <= files.size() &&
                fileRecordSize <= files.size() - offset);
        auto result = DARC_FILEHEAD_VER5{};
        result.CompressedDataSize = 0xffffffff;
        std::memcpy(&result, files.data() + offset, fileRecordSize);
        return result;
    };
    auto filename = [&](size_t offset) {
        const auto original =
          offset + 4 + size_t(record<u16>(names, offset)) * 4;
        require(original < names.size());
        const auto end = std::find(names.begin() + original, names.end(), u8{});
        require(end != names.end());
        auto result =
          std::string(reinterpret_cast<const char*>(names.data() + original),
                      static_cast<size_t>(end - (names.begin() + original)));
        require(!result.empty() && result != "." && result != ".." &&
                result.find_first_of("/\\:") == std::string::npos);
        return result;
    };
    struct Directory
    {
        size_t offset;
        std::filesystem::path path;
    };
    auto pending = std::vector<Directory>{ { 0, {} } };
    auto visited = std::unordered_set<size_t>{};
    auto output = support::DXArchive{};
    while (!pending.empty()) {
        auto current = std::move(pending.back());
        pending.pop_back();
        require(visited.insert(current.offset).second);
        const auto directory =
          record<DARC_DIRECTORY_VER5>(directories, current.offset);
        if (directory.DirectoryAddress != 0xffffffff &&
            directory.ParentDirectoryAddress != 0xffffffff) {
            const auto entry = fileRecord(directory.DirectoryAddress);
            current.path /= filename(entry.NameAddress);
        }
        require(directory.FileHeadAddress <= files.size());
        require(directory.FileHeadNum <=
                (files.size() - directory.FileHeadAddress) / fileRecordSize);
        for (size_t i = 0; i < directory.FileHeadNum; ++i) {
            const auto entry =
              fileRecord(directory.FileHeadAddress + i * fileRecordSize);
            if (entry.Attributes & FILE_ATTRIBUTE_DIRECTORY) {
                pending.push_back({ entry.DataAddress, current.path });
                continue;
            }
            const auto entryPath = current.path / filename(entry.NameAddress);
            auto segment = support::DXArchiveSegment{ entry.DataSize, {} };
            if (entry.DataSize != 0) {
                const auto offset =
                  uint64_t(header.DataStartAddress) + entry.DataAddress;
                const auto compressed =
                  header.Version >= 2 && entry.CompressedDataSize != 0xffffffff;
                range(offset,
                      compressed ? entry.CompressedDataSize : entry.DataSize);
                file.seekg(static_cast<std::streamoff>(offset));
                const auto keyPosition =
                  header.Version >= 5
                    ? std::optional<uint64_t>{ entry.DataSize }
                    : std::nullopt;
                if (compressed) {
                    auto encoded = std::vector<u8>(entry.CompressedDataSize);
                    KeyConvFileRead(encoded.data(),
                                    encoded.size(),
                                    file,
                                    key.data(),
                                    keyPosition);
                    Decompress(encoded, {}, entry.DataSize);
                    segment.data =
                      std::shared_ptr<u8[]>(new u8[entry.DataSize]);
                    Decompress(encoded,
                               { segment.data.get(), segment.size },
                               entry.DataSize);
                } else {
                    segment.data =
                      std::shared_ptr<u8[]>(new u8[entry.DataSize]);
                    KeyConvFileRead(segment.data.get(),
                                    segment.size,
                                    file,
                                    key.data(),
                                    keyPosition);
                }
            }
            output[support::normalizeDxaPath(entryPath)] = std::move(segment);
        }
    }
    return output;
}

} // namespace dxa

namespace support {
auto
normalizeDxaPath(const std::filesystem::path& path) -> std::string
{
    auto normalized = support::pathToUtfString(path);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    normalized =
      std::filesystem::path(normalized).lexically_normal().generic_string();

    while (normalized.starts_with("./")) {
        normalized.erase(0, 2);
    }
    if (normalized == ".") {
        normalized.clear();
    }

    return normalized;
}

DXArchive
extractDxaToMem(const std::filesystem::path& path)
{
    try {
        if (!std::filesystem::is_regular_file(path)) {
            return {};
        }
        return dxa::DecodeArchive(path);
    } catch (const std::exception& error) {
        spdlog::warn("Could not read DXA {}: {}",
                     support::pathToUtfString(path),
                     error.what());
        return {};
    }
}

int
extractDxaToFile(const std::filesystem::path& path)
{
    try {
        const auto archive = dxa::DecodeArchive(path);
        const auto directory = path.parent_path() / path.stem();
        std::filesystem::create_directories(directory);
        for (const auto& [name, contents] : archive) {
            const auto target = directory / support::utfStringToPath(name);
            if (std::filesystem::is_regular_file(target)) {
                continue;
            }
            std::filesystem::create_directories(target.parent_path());
            std::ofstream file(target, std::ios::binary);
            if (contents.size) {
                file.write(reinterpret_cast<const char*>(contents.data.get()),
                           static_cast<std::streamsize>(contents.size));
            }
            if (!file) {
                throw std::runtime_error("Could not write extracted DXA file");
            }
        }
        return 0;
    } catch (const std::exception& error) {
        spdlog::warn("Could not extract DXA {}: {}",
                     support::pathToUtfString(path),
                     error.what());
        return -1;
    }
}
} // namespace support
