// license:BSD-3-Clause
// copyright-holders:F. Ulivi
//
// *****************************************
// Emulator for HP "hybrid" processor series
// *****************************************
//
// The HP hybrid processor series is composed of a few different models with different
// capabilities. The series was derived from HP's own 2116 processor by translating a
// discrete implementation of the 1960s into a multi-chip module (hence the "hybrid" name).
// This emulator currently supports the 5061-3011 version only.
//
// For this emulator I mainly relied on these sources:
// - http://www.hp9845.net/ website
// - HP manual "Assembly development ROM manual for the HP9845": this is the most precious
//   and "enabling" resource of all
// - US Patent 4,180,854 describing the HP9845 system
// - Study of disassembly of firmware of HP64000 system
// - A lot of "educated" guessing

#ifndef _HPHYBRID_H_
#define _HPHYBRID_H_

// Input lines
#define HPHYBRID_IRH    0       // High-level interrupt
#define HPHYBRID_IRL    1       // Low-level interrupt
#define HPHYBRID_INT_LVLS   2   // Levels of interrupt

#define HPHYBRID_DMAR   2       // DMA request
#define HPHYBRID_HALT   3       // "Halt" input
#define HPHYBRID_STS    4       // "Status" input
#define HPHYBRID_FLG    5       // "Flag" input

// I/O addressing space (16-bit wide)
// Addresses into this space are composed as follows:
// b[5..2] = Peripheral address 0..15
// b[1..0] = Register address (IC) 0..3
#define HP_IOADDR_PA_SHIFT      2
#define HP_IOADDR_IC_SHIFT      0

// Compose an I/O address from PA & IC
#define HP_MAKE_IOADDR(pa , ic)    (((pa) << HP_IOADDR_PA_SHIFT) | ((ic) << HP_IOADDR_IC_SHIFT))

// Addresses of memory mapped registers
#define HP_REG_A_ADDR   0x0000
#define HP_REG_B_ADDR   0x0001
#define HP_REG_P_ADDR   0x0002
#define HP_REG_R_ADDR   0x0003
#define HP_REG_R4_ADDR  0x0004
#define HP_REG_R5_ADDR  0x0005
#define HP_REG_R6_ADDR  0x0006
#define HP_REG_R7_ADDR  0x0007
#define HP_REG_IV_ADDR  0x0008
#define HP_REG_PA_ADDR  0x0009
#define HP_REG_W_ADDR   0x000A
#define HP_REG_DMAPA_ADDR       0x000B
#define HP_REG_DMAMA_ADDR       0x000C
#define HP_REG_DMAC_ADDR        0x000D
#define HP_REG_C_ADDR   0x000e
#define HP_REG_D_ADDR   0x000f
#define HP_REG_AR2_ADDR 0x0010
#define HP_REG_SE_ADDR  0x0014
#define HP_REG_R25_ADDR 0x0015
#define HP_REG_R26_ADDR 0x0016
#define HP_REG_R27_ADDR 0x0017
#define HP_REG_R32_ADDR 0x001a
#define HP_REG_R33_ADDR 0x001b
#define HP_REG_R34_ADDR 0x001c
#define HP_REG_R35_ADDR 0x001d
#define HP_REG_R36_ADDR 0x001e
#define HP_REG_R37_ADDR 0x001f
#define HP_REG_LAST_ADDR        0x001f
#define HP_REG_AR1_ADDR 0xfff8

#define HP_REG_IV_MASK  0xfff0
#define HP_REG_PA_MASK  0x000f

class hp_hybrid_cpu_device : public cpu_device
{
public:
                DECLARE_WRITE_LINE_MEMBER(dmar_w);
                DECLARE_WRITE_LINE_MEMBER(halt_w);
                DECLARE_WRITE_LINE_MEMBER(status_w);
                DECLARE_WRITE_LINE_MEMBER(flag_w);

protected:
        hp_hybrid_cpu_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname , UINT8 addrwidth);

                // device-level overrides
                virtual void device_start();
                virtual void device_reset();

                // device_execute_interface overrides
                virtual UINT32 execute_min_cycles() const { return 6; }
                virtual UINT32 execute_input_lines() const { return 2; }
                virtual UINT32 execute_default_irq_vector() const { return 0xffff; }
                virtual void execute_run();
                virtual void execute_set_input(int inputnum, int state);

                UINT16 execute_one(UINT16 opcode);
                UINT16 execute_one_sub(UINT16 opcode);
        // Execute an instruction that doesn't belong to either BPC or IOC
        virtual UINT16 execute_no_bpc_ioc(UINT16 opcode) = 0;

                // device_memory_interface overrides
                virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const { return (spacenum == AS_PROGRAM) ? &m_program_config : ( (spacenum == AS_IO) ? &m_io_config : NULL ); }

                // device_state_interface overrides
                void state_string_export(const device_state_entry &entry, std::string &str);

                // device_disasm_interface overrides
                virtual UINT32 disasm_min_opcode_bytes() const { return 2; }
                virtual UINT32 disasm_max_opcode_bytes() const { return 2; }
                virtual offs_t disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options);

        // Different cases of memory access
        // See patent @ pg 361
        typedef enum {
                AEC_CASE_A,     // Instr. fetches, non-base page fetches of link pointers, BPC direct non-base page accesses
                AEC_CASE_B,     // Base page fetches of link pointers, BPC direct base page accesses
                AEC_CASE_C,     // IOC, EMC & BPC indirect final destination accesses
                AEC_CASE_D      // DMA accesses
        } aec_cases_t;

        // do memory address extension
        virtual UINT32 add_mae(aec_cases_t aec_case , UINT16 addr) = 0;

        UINT16 remove_mae(UINT32 addr);

        UINT16 RM(aec_cases_t aec_case , UINT16 addr);
        UINT16 RM(UINT32 addr);
        virtual UINT16 read_non_common_reg(UINT16 addr) = 0;

        void   WM(aec_cases_t aec_case , UINT16 addr , UINT16 v);
        void   WM(UINT32 addr , UINT16 v);
        virtual void write_non_common_reg(UINT16 addr , UINT16 v) = 0;

        UINT16 fetch(void);
        
                int m_icount;

                // State of processor
                UINT16 m_reg_A;     // Register A
                UINT16 m_reg_B;     // Register B
                UINT16 m_reg_P;     // Register P
                UINT16 m_reg_R;     // Register R
                UINT16 m_reg_C;     // Register C
                UINT16 m_reg_D;     // Register D
                UINT16 m_reg_IV;    // Register IV
        UINT16 m_reg_W; // Register W
                UINT8  m_reg_PA[ HPHYBRID_INT_LVLS + 1 ];   // Stack of register PA (4 bit-long)
                UINT16 m_flags;     // Flags
                UINT8  m_dmapa;     // DMA peripheral address (4 bits)
                UINT16 m_dmama;     // DMA address
                UINT16 m_dmac;      // DMA counter
                UINT16 m_reg_I;     // Instruction register
        UINT32 m_genpc; // Full PC

private:
                address_space_config m_program_config;
                address_space_config m_io_config;

                address_space *m_program;
                direct_read_data *m_direct;
                address_space *m_io;

                UINT32 get_ea(UINT16 opcode);
                void do_add(UINT16& addend1 , UINT16 addend2);
                UINT16 get_skip_addr(UINT16 opcode , bool condition) const;
                UINT16 get_skip_addr_sc(UINT16 opcode , UINT16& v , unsigned n);
                void do_pw(UINT16 opcode);
                void check_for_interrupts(void);
                                void handle_dma(void);

                UINT16 RIO(UINT8 pa , UINT8 ic);
                void   WIO(UINT8 pa , UINT8 ic , UINT16 v);
};

class hp_5061_3001_cpu_device : public hp_hybrid_cpu_device
{
public:
        hp_5061_3001_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
        virtual void device_start();
        virtual void device_reset();
        virtual UINT32 execute_max_cycles() const { return 213; }       // XFR 16
        virtual UINT16 execute_no_bpc_ioc(UINT16 opcode);
        virtual offs_t disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options);
        virtual UINT32 add_mae(aec_cases_t aec_case , UINT16 addr);
        virtual UINT16 read_non_common_reg(UINT16 addr);
        virtual void write_non_common_reg(UINT16 addr , UINT16 v);

private:
        // Additional state of processor
        UINT16 m_reg_ar2[ 4 ];  // AR2 register
        UINT16 m_reg_se;        // SE register (4 bits)
        UINT16 m_reg_r25;       // R25 register
        UINT16 m_reg_r26;       // R26 register
        UINT16 m_reg_r27;       // R27 register
        UINT16 m_reg_aec[ HP_REG_R37_ADDR - HP_REG_R32_ADDR + 1 ];      // AEC registers R32-R37
};

class hp_5061_3011_cpu_device : public hp_hybrid_cpu_device
{
public:
        hp_5061_3011_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
        virtual UINT32 execute_max_cycles() const { return 25; }
        virtual UINT16 execute_no_bpc_ioc(UINT16 opcode);
        virtual UINT32 add_mae(aec_cases_t aec_case , UINT16 addr);
        virtual UINT16 read_non_common_reg(UINT16 addr);
        virtual void write_non_common_reg(UINT16 addr , UINT16 v);

};

extern const device_type HP_5061_3001;
extern const device_type HP_5061_3011;

#endif /* _HPHYBRID_H_ */
