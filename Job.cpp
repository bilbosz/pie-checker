//
// Created by adam on 02.06.23.
//

#include "Job.h"

#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>

#include <elf.h>

#ifndef DF_1_PIE
#define DF_1_PIE 0x08000000
#endif

struct FileInfo {
    enum class Type : uint8_t {
        NotElf = ET_NONE,
        ElfRelocatableFile = ET_REL,
        ElfExecutableFile = ET_EXEC,
        ElfSharedObject = ET_DYN,
        ElfCoreFile = ET_CORE
    } type = Type::NotElf;
    enum class Architecture : uint8_t {
        None = ELFCLASSNONE,
        X32 = ELFCLASS32,
        X64 = ELFCLASS64
    } architecture = Architecture::None;
    enum class Endianness : uint8_t {
        None = ELFDATANONE,
        LittleEndian = ELFDATA2LSB,
        BigEndian = ELFDATA2MSB
    } endianness = Endianness::None;
    enum class Version : uint8_t {
        None = EV_NONE,
        Current = EV_CURRENT
    } version = Version::None;
    enum class OsAbi : uint8_t {
        None = ELFOSABI_NONE,
        Sysv = ELFOSABI_SYSV,
        HpUx = ELFOSABI_HPUX,
        NetBsd = ELFOSABI_NETBSD,
        Linux = ELFOSABI_LINUX,
        Solaris = ELFOSABI_SOLARIS,
        Irix = ELFOSABI_IRIX,
        FreeBsd = ELFOSABI_FREEBSD,
        Tru64 = ELFOSABI_TRU64,
        Arm = ELFOSABI_ARM,
        Standalone = ELFOSABI_STANDALONE
    } osAbi = OsAbi::None;
    uint8_t abiVersion = 0;
    enum class Machine {
        None = EM_NONE,
        M32 = EM_M32,
        Sparc = EM_SPARC,
        Intel80386 = EM_386,
        Motorola68K = EM_68K,
        Motorola88K = EM_88K,
        Intel80860 = EM_860,
        MipsRs3000 = EM_MIPS,
        HpPa = EM_PARISC,
        SparcEis = EM_SPARC32PLUS,
        PowerPc = EM_PPC,
        PowerPc64 = EM_PPC64,
        IbmS390 = EM_S390,
        Arm = EM_ARM,
        RenesansSuperH = EM_SH,
        SparcV964 = EM_SPARCV9,
        IntelItanium = EM_IA_64,
        Amd8664 = EM_X86_64,
        DecVax = EM_VAX
    } machine = Machine::None;
    bool isPieLinked = false;
};

std::ostream &operator<<(std::ostream &os, const FileInfo &fi) {
    switch (fi.type) {
        case FileInfo::Type::NotElf:
            os << "not_elf;";
            break;
        case FileInfo::Type::ElfRelocatableFile:
            os << "elf_relocatable_file;";
            break;
        case FileInfo::Type::ElfExecutableFile:
            os << "elf_executable_file;";
            break;
        case FileInfo::Type::ElfSharedObject:
            os << "elf_shared_object;";
            break;
        case FileInfo::Type::ElfCoreFile:
            os << "elf_core_file;";
            break;
    }
    switch (fi.architecture) {
        case FileInfo::Architecture::None:
            os << "unknown;";
            break;
        case FileInfo::Architecture::X32:
            os << "32_bit;";
            break;
        case FileInfo::Architecture::X64:
            os << "64_bit;";
            break;
    }
    switch (fi.endianness) {
        case FileInfo::Endianness::None:
            os << "unknown;";
            break;
        case FileInfo::Endianness::LittleEndian:
            os << "lsb;";
            break;
        case FileInfo::Endianness::BigEndian:
            os << "msb;";
            break;
    }
    os << (fi.isPieLinked ? "pie" : "not_pie");
    return os;
}

template<std::integral T>
constexpr T ByteSwap(T value) noexcept {
#ifdef __cpp_lib_byteswap
    return std::byteswap(value);
#else
    auto bytes = reinterpret_cast<char *>(&value);
    for (int i = 0; i < sizeof(value) / 2; ++i) {
        std::swap(bytes[i], bytes[sizeof(value) - 1 - i]);
    }
    return *reinterpret_cast<T *>(bytes);
#endif
}

template<FileInfo::Endianness fileInfoEndian, std::integral T>
T ByteSwapIfNeeded(T value) {
    constexpr auto fileEndian = (fileInfoEndian == FileInfo::Endianness::LittleEndian ? std::endian::little
                                                                                      : std::endian::big);
    if constexpr (std::endian::native != fileEndian)
        return ByteSwap(value);
    return value;
}

template<FileInfo::Architecture arch, FileInfo::Endianness endian>
static FileInfo CheckPieImpl(const char ident[EI_NIDENT], std::ifstream &ifs) {
#define S(x) ByteSwapIfNeeded<endian>(x)
    FileInfo fileInfo = {};

    using Ehdr = std::conditional_t<arch == FileInfo::Architecture::X32, Elf32_Ehdr, Elf64_Ehdr>;
    Ehdr elfHeader;
    memcpy((void *) &elfHeader, ident, EI_NIDENT);
    if (!ifs.read(reinterpret_cast<char *>(&elfHeader) + EI_NIDENT, sizeof(elfHeader) - EI_NIDENT))
        return {};
    fileInfo.architecture = static_cast<FileInfo::Architecture>(elfHeader.e_ident[EI_CLASS]);
    fileInfo.endianness = static_cast<FileInfo::Endianness>(elfHeader.e_ident[EI_DATA]);
    fileInfo.version = static_cast<FileInfo::Version>(elfHeader.e_ident[EI_VERSION]);
    fileInfo.osAbi = static_cast<FileInfo::OsAbi>(elfHeader.e_ident[EI_OSABI]);
    fileInfo.abiVersion = elfHeader.e_ident[EI_ABIVERSION];
    fileInfo.type = static_cast<FileInfo::Type>(S(elfHeader.e_type));
    fileInfo.machine = static_cast<FileInfo::Machine>(S(elfHeader.e_machine));

    for (int i = 0; i < S(elfHeader.e_shnum); ++i) {
        if (!ifs.seekg(S(elfHeader.e_shoff) + i * S(elfHeader.e_shentsize)))
            return {};
        using Shdr = std::conditional_t<arch == FileInfo::Architecture::X32, Elf32_Shdr, Elf64_Shdr>;
        Shdr sectionHeader;
        if (!ifs.read(reinterpret_cast<char *>(&sectionHeader), sizeof(sectionHeader)))
            return {};
        if (S(sectionHeader.sh_type) == SHT_DYNAMIC) {
            if (!ifs.seekg(S(sectionHeader.sh_offset)))
                return {};
            // dynamic section header
            using Dyn = std::conditional_t<arch == FileInfo::Architecture::X32, Elf32_Dyn, Elf64_Dyn>;
            Dyn dynamicSectionHeader;
            if (!ifs.read(reinterpret_cast<char *>(&dynamicSectionHeader), sizeof(dynamicSectionHeader)))
                return {};
            for (decltype(dynamicSectionHeader.d_tag) tag = S(dynamicSectionHeader.d_tag);
                 tag != DT_NULL; tag = S(dynamicSectionHeader.d_tag)) {
                if (tag == DT_FLAGS_1) {
                    if (S(dynamicSectionHeader.d_un.d_val) & DF_1_PIE) {
                        fileInfo.isPieLinked = true;
                        return fileInfo;
                    }
                }
                if (!ifs.read(reinterpret_cast<char *>(&dynamicSectionHeader), sizeof(dynamicSectionHeader)))
                    return {};
            }
        }
    }
    return fileInfo;
#undef S
}

static FileInfo CheckPie(const std::string &filename) {
    std::ifstream ifs(filename, std::ios_base::binary);
    char ident[EI_NIDENT];
    if (!ifs.read(ident, EI_NIDENT))
        return {};
    if (!std::equal(ident, ident + SELFMAG, ELFMAG))
        return {};
    switch (ident[EI_DATA]) {
        case ELFDATA2LSB:
            switch (ident[EI_CLASS]) {
                case ELFCLASS32:
                    return CheckPieImpl<FileInfo::Architecture::X32, FileInfo::Endianness::LittleEndian>(ident, ifs);
                case ELFCLASS64:
                    return CheckPieImpl<FileInfo::Architecture::X64, FileInfo::Endianness::LittleEndian>(ident, ifs);
                default:
                    return {};
            }
        case ELFDATA2MSB:
            switch (ident[EI_CLASS]) {
                case ELFCLASS32:
                    return CheckPieImpl<FileInfo::Architecture::X32, FileInfo::Endianness::BigEndian>(ident, ifs);
                case ELFCLASS64:
                    return CheckPieImpl<FileInfo::Architecture::X64, FileInfo::Endianness::BigEndian>(ident, ifs);
                default:
                    return {};
            }
    }
    return {};
}

static std::mutex ioMutex;

void Job(const std::string &file) {
    auto fileInfo = CheckPie(file);
    std::lock_guard lock(ioMutex);
    std::cout << file << ";" << fileInfo << '\n';
}