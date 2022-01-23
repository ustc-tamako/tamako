#ifndef INCLUDE_ELF_H_
#define INCLUDE_ELF_H_

#include "types.h"
#include "multiboot.h"

// ELF 格式区段头
typedef
struct elf_section_header_t {
	uint32_t name;    // 在 .strtab 中的偏移
	uint32_t type;
	uint32_t flags;
	uint32_t addr;    // section 首址
	uint32_t offset;
	uint32_t size;    // section 大小
	uint32_t link;
	uint32_t info;
	uint32_t addralign;
	uint32_t entsize;
} __attribute__((packed)) elf_section_header_t;

// ELF 格式符号
typedef
struct elf_symbol_t {
	uint32_t name;
	uint32_t value;
	uint32_t size;
	uint8_t  info;
	uint8_t  other;
	uint16_t shndx;
} __attribute__((packed)) elf_symbol_t;

// ELF 信息，保存了符号表信息和对应的符号字符串表
typedef
struct elf_t {
	elf_symbol_t * symtab;
	uint32_t       symtabsz;
	const char   * strtab;
	uint32_t       strtabsz;
} elf_t;

// 从 multiboot_t 结构获取ELF信息
elf_t elf_from_multiboot(multiboot_t * mb);

// 查看ELF的符号信息
const char * elf_lookup_symbol(uint32_t addr, elf_t * elf);

#endif  // INCLUDE_ELF_H_