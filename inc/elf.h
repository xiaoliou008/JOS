#ifndef JOS_INC_ELF_H
#define JOS_INC_ELF_H

#define ELF_MAGIC 0x464C457FU	/* "\x7FELF" in little endian */	/// ASCII!

struct Elf {				// 这个数据结构是ELF头部
	uint32_t e_magic;		// must equal ELF_MAGIC
	uint8_t e_elf[12];		// 系统字大小、字节顺序
	uint16_t e_type;		// 文件类型
	uint16_t e_machine;		// 运行该程序需要的体系结构
	uint32_t e_version; 	// 文件的版本
	uint32_t e_entry;		// 程序的入口地址
	uint32_t e_phoff;		// 表示Program header table 在文件中的偏移量
	uint32_t e_shoff;		// 表示Section header table 在文件中的偏移量
	uint32_t e_flags;		// 对IA32而言，此项为0
	uint16_t e_ehsize;		// 表示ELF header大小（以字节计数）
	uint16_t e_phentsize;	// 表示Program header table中每一个条目的大小
	uint16_t e_phnum;		// 表示Program header table中有多少个条目
	uint16_t e_shentsize;	// 表示Section header table中每一个条目的大小
	uint16_t e_shnum;		// 表示Section header table中有多少个条目
	uint16_t e_shstrndx;	// 包含节名称的字符串是第几个节（从零开始计数）
};

struct Proghdr {			// Program header描述一个段的位置和大小
	uint32_t p_type;		// 当前描述的段的类型
	uint32_t p_offset;		// 段的第一个字节在文件中的偏移
	uint32_t p_va;			// 段的一个字节在内存中的虚拟地址
	uint32_t p_pa;			// 此项是为物理地址保留
	uint32_t p_filesz;		// 段在文件中的长度
	uint32_t p_memsz;		// 段在内存中的长度
	uint32_t p_flags;		// 与段相关的标志
	uint32_t p_align;		// 根据此项值来确定段在文件及内存中如何对齐
};

struct Secthdr {
	uint32_t sh_name;		// 节的名称
	uint32_t sh_type;		// 节的类型
	uint32_t sh_flags;		// 节的标记
	uint32_t sh_addr;		// 节映射到虚拟地址空间中的位置
	uint32_t sh_offset;		// 节在文件中的起始位置
	uint32_t sh_size;		// 节的长度
	uint32_t sh_link;		// 引用另一个节头表项
	uint32_t sh_info;		// 与sh_link联用
	uint32_t sh_addralign;	// 节数据在内存中的对齐方式
	uint32_t sh_entsize;	// 指定了节中各数据项的长度
};

// Values for Proghdr::p_type
#define ELF_PROG_LOAD		1

// Flag bits for Proghdr::p_flags
#define ELF_PROG_FLAG_EXEC	1
#define ELF_PROG_FLAG_WRITE	2
#define ELF_PROG_FLAG_READ	4

// Values for Secthdr::sh_type
#define ELF_SHT_NULL		0
#define ELF_SHT_PROGBITS	1
#define ELF_SHT_SYMTAB		2
#define ELF_SHT_STRTAB		3

// Values for Secthdr::sh_name
#define ELF_SHN_UNDEF		0

#endif /* !JOS_INC_ELF_H */
