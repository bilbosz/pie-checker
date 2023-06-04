//
// Created by adam on 02.06.23.
//

#include "Job.h"

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

template<FileInfo::Architecture Arch>
static FileInfo CheckPieImpl(const char ident[EI_NIDENT], std::ifstream &ifs) {
    FileInfo fileInfo = {};

    using Ehdr = std::conditional_t<Arch == FileInfo::Architecture::X32, Elf32_Ehdr, Elf64_Ehdr>;
    Ehdr elfHeader;
    memcpy((void *) &elfHeader, ident, EI_NIDENT);
    if (!ifs.read(reinterpret_cast<char *>(&elfHeader) + EI_NIDENT, sizeof(elfHeader) - EI_NIDENT))
        return {};
    fileInfo.architecture = static_cast<FileInfo::Architecture>(elfHeader.e_ident[EI_CLASS]);
    fileInfo.endianness = static_cast<FileInfo::Endianness>(elfHeader.e_ident[EI_DATA]);
    fileInfo.version = static_cast<FileInfo::Version>(elfHeader.e_ident[EI_VERSION]);
    fileInfo.osAbi = static_cast<FileInfo::OsAbi>(elfHeader.e_ident[EI_OSABI]);
    fileInfo.abiVersion = elfHeader.e_ident[EI_ABIVERSION];
    fileInfo.type = static_cast<FileInfo::Type>(elfHeader.e_type);
    fileInfo.machine = static_cast<FileInfo::Machine>(elfHeader.e_machine);

    for (int i = 0; i < elfHeader.e_shnum; ++i) {
        if (!ifs.seekg(elfHeader.e_shoff + i * elfHeader.e_shentsize))
            return {};
        using Shdr = std::conditional_t<Arch == FileInfo::Architecture::X32, Elf32_Shdr, Elf64_Shdr>;
        Shdr sectionHeader;
        if (!ifs.read(reinterpret_cast<char *>(&sectionHeader), sizeof(sectionHeader)))
            return {};
        if (sectionHeader.sh_type == SHT_DYNAMIC) {
            if (!ifs.seekg(sectionHeader.sh_offset))
                return {};
            // dynamic section header
            using Dyn = std::conditional_t<Arch == FileInfo::Architecture::X32, Elf32_Dyn, Elf64_Dyn>;
            Dyn dynamicSectionHeader;
            if (!ifs.read(reinterpret_cast<char *>(&dynamicSectionHeader), sizeof(dynamicSectionHeader)))
                return {};
            for (decltype(dynamicSectionHeader.d_tag) tag = dynamicSectionHeader.d_tag;
                 tag != DT_NULL; tag = dynamicSectionHeader.d_tag) {
                if (tag == DT_FLAGS_1) {
                    if (dynamicSectionHeader.d_un.d_val & DF_1_PIE) {
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
}

static FileInfo CheckPie(const std::string &filename) {
    std::ifstream ifs(filename, std::ios_base::binary);
    char ident[EI_NIDENT];
    if (!ifs.read(ident, EI_NIDENT))
        return {};
    if (!std::equal(ident, ident + SELFMAG, ELFMAG))
        return {};
    switch (ident[EI_CLASS]) {
        case ELFCLASS32:
            return CheckPieImpl<FileInfo::Architecture::X32>(ident, ifs);
        case ELFCLASS64:
            return CheckPieImpl<FileInfo::Architecture::X64>(ident, ifs);
    }
    return {};
}

static std::mutex ioMutex;

void Job(const std::string &file) {
    auto fileInfo = CheckPie(file);
    if (fileInfo.type != FileInfo::Type::NotElf) {
        std::lock_guard lock(ioMutex);
        std::cout << "\"" << file << "\" is " << (fileInfo.isPieLinked ? "PIE" : "not PIE") << std::endl;
    }
}