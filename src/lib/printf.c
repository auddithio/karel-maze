#include "printf.h"
#include <stdarg.h>
#include <stdint.h>
#include "strings.h"
#include "uart.h"

/** Prototypes for internal helpers.
 * Typically these would be qualified as static (private to module)
 * but, in order to call them from the test program, we declare them externally
 */
int unsigned_to_base(char *buf, 
                     size_t bufsize, 
                     unsigned int val, 
                     int base, size_t 
                     min_width);
int signed_to_base(char *buf, 
                   size_t bufsize, 
                   int val, 
                   int base, 
                   size_t min_width);
int disassemble(char *buf, int bufsize, unsigned int *addr);

#define MAX_OUTPUT_LEN 1024

const unsigned int ASCII_BASE10 = 48;
const unsigned int ASCII_BASE16_LOWER = 87;

int unsigned_to_base(char *buf, size_t bufsize, unsigned int val, int base, size_t min_width)
{   
    // find number of digits in val;
    int num_digits = 0;
    unsigned int copy = val;
    while (1) {
        num_digits++;
        copy /= base;
        if (copy == 0) break;
    }

    // determine number of digits we have to write
    if (min_width > num_digits) {
       num_digits = min_width;
    }

    char temp[num_digits + 1]; // temporary array, + 1 for null terminator
    temp[num_digits] = '\0';  

    // populate temp with numbers
    int index = num_digits - 1;
    while (index >= 0) { // pads 0's if needed

        // determine offset to convert to ASCII
        int offset = ASCII_BASE10;
        if (val % base > 9) {
            offset = ASCII_BASE16_LOWER;
        }

        temp[index--] = (char) (val % base + offset);
        val /= base;
    }

    *buf = '\0'; // initialise buf as empty
    strlcat(buf, temp, bufsize);
    return num_digits;

}

int signed_to_base(char *buf, size_t bufsize, int val, int base, size_t min_width)
{
    *buf = '\0'; // initialise buf
    int num_digits = 0; // return value

    if (val < 0) { 
        val = -val;
        min_width == 0 ? 0 : min_width--; // 1 fewer padded zero needed
        num_digits++;
        strlcat(buf, "-", bufsize); // add the minus sign
    }
    
    // create a temporary array for the part without minus sign
    char temp[bufsize];
    num_digits += unsigned_to_base(temp, bufsize, val, base, min_width);

    strlcat(buf, temp, bufsize); 
    return num_digits;
}

int vsnprintf(char *buf, size_t bufsize, const char *format, va_list args)
{
    int length = 0;
    char temp[MAX_OUTPUT_LEN + 1]; // temporary, + 1 for null terminator
    *temp = '\0';

    // traverse string until we reach end of format or buffer
    while (*format && length < MAX_OUTPUT_LEN - 1) { 

        // if % sign is involved
        if (*format == '%') {

            // check if there's a number after %
            int num = strtonum(++format, &format); 
            
            if (*format == '%') { // if another % sign
                temp[length++] = *format;
            
            } else if (*format == 'c') { // character
                temp[length++] = (char) va_arg(args, int);
                            
            } else if (*format == 's') { // string
                temp[length] = '\0'; 
                length += strlcat(temp + length, (char *)va_arg(args, int*), 
                                 MAX_OUTPUT_LEN);

            } else if (*format == 'd') { // decimal number
                length += signed_to_base(temp + length, MAX_OUTPUT_LEN, 
                                        va_arg(args, int), 10, num);
            
            } else if (*format == 'x') { // hex number
                length += unsigned_to_base(temp + length, MAX_OUTPUT_LEN, 
                                           va_arg(args, int), 16, num);

            } else if (*format == 'p') { // pointer
                temp[length] = '\0';

                // disassembler
                if (*(format + 1) == 'I') {
                    format++;
                    length += disassemble(temp + length, MAX_OUTPUT_LEN, 
                                            va_arg(args, unsigned int*));

                } else {

                    // append '0x'
                    length += strlcat(temp + length, "0x", MAX_OUTPUT_LEN);

                    // append hex pointer value
                    length += unsigned_to_base(temp + length, MAX_OUTPUT_LEN, 
                                                va_arg(args, int), 16, 0);
                }
            }

            format++;

        } else {
            temp[length++] = *format++; // copy and increment
        }

    }

    // terminate array
    if (length > MAX_OUTPUT_LEN) {
        temp[MAX_OUTPUT_LEN] = '\0';
    } else {
        temp[length] = '\0';
    }

    // copy to buf
    *buf = '\0';
    strlcat(buf, temp, bufsize); 

    return length;
}

int snprintf(char *buf, size_t bufsize, const char *format, ...)
{
    //set up varadic arguments
    va_list args;
    va_start(args, format);

    int length = vsnprintf(buf, bufsize, format, args);

    va_end(args);
    return length;
}

int printf(const char *format, ...)
{
    char buf[MAX_OUTPUT_LEN + 1]; // maximum array

    va_list args;
    va_start(args, format);

    // populate buf
    int length = vsnprintf(buf, MAX_OUTPUT_LEN + 1, format, args);
    va_end(args);

    // print to console
    uart_putstring(buf);

    return length;
}

/* From here to end of file is some sample code and suggested approach
 * for those of you doing the disassemble extension. Otherwise, ignore!
 *
 * The struct insn bitfield is declared using exact same layout as bits are organized in
 * the encoded instruction. Accessing struct.field will extract just the bits
 * apportioned to that field. If you look at the assembly the compiler generates
 * to access a bitfield, you will see it simply masks/shifts for you. Neat!
 */


static const char *cond[16] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
                               "hi", "ls", "ge", "lt", "gt", "le", "", ""};
static const char *opcodes[16] = {"and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
                                  "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"};

struct insn  {
    uint32_t reg_op2:4;
    uint32_t one:1;
    uint32_t shift_op: 2;
    uint32_t shift: 5;
    uint32_t reg_dst:4;
    uint32_t reg_op1:4;
    uint32_t s:1;
    uint32_t opcode:4;
    uint32_t imm:1;
    uint32_t kind:2;
    uint32_t cond:4;
};

struct branch {
    int32_t imm24:24;
    uint32_t funct:2;
    uint32_t op:2;
    uint32_t cond:4;
};

static void sample_use(unsigned int *addr) {
    struct insn in = *(struct insn *)addr;
    printf("opcode is %s, s is %d, reg_dst is r%d\n", opcodes[in.opcode], in.s, in.reg_dst);
}

/*
 * Adds register name to the buffer
 */
int add_reg_name(char *buf, int bufsize, int reg_num) {
    if (reg_num == 15) {
        return strlcat(buf, "pc", bufsize);
    } else if (reg_num == 14) {
        return strlcat(buf, "lr", bufsize);
    } else if (reg_num == 13) {
        return strlcat(buf, "sp", bufsize);
    } else if (reg_num == 11) {
        return strlcat(buf, "fp", bufsize);
    } else {
        int length = 0;
        length += strlcat(buf + length, "r", bufsize);
        length += unsigned_to_base(buf + length, bufsize, reg_num, 10, 1); 
        return length;
    }

}

/*
 * Sets up treh disassembling
 * @precon  buf must be null terminated
 */
int disassemble(char *buf, int bufsize, unsigned int *addr) {
    struct insn in = *(struct insn *)addr;
    int length = 0;
    
    // simple instruction
    if (in.kind == 0) { 
        // 'instr dst, op1, op2'
        length += strlcat(buf + length, opcodes[in.opcode], bufsize); // name
        length += strlcat(buf + length, cond[in.cond], bufsize); // conditions
        length += strlcat(buf + length, " ", bufsize);

        // no destination register
        if (in.opcode < 0b1000 || in.opcode > 0b1011) {
            length += add_reg_name(buf + length, bufsize, in.reg_dst);
            length += strlcat(buf + length, ", ", bufsize);
        }

        // not move
        if (in.opcode != 0b1101) {
            length += add_reg_name(buf + length, bufsize, in.reg_op1);
            length += strlcat(buf + length, ", ", bufsize);
        }

        // immediate value or not
        if (in.imm == 0) {
            length += add_reg_name(buf + length, bufsize, in.reg_op2);
        } else {
            length += strlcat(buf + length, "#", bufsize);
            length += unsigned_to_base(buf + length, bufsize, *addr & 0xff, 10, 1);
        }

    } else if (in.kind == 0b10) { // branching
        struct branch br= *(struct branch *)addr;
        length += strlcat(buf + length, "b", bufsize);
        length += strlcat(buf + length, cond[br.cond], bufsize); // conditions
        length += strlcat(buf + length, " ", bufsize);
        int bit_shift = br.imm24 << 2;
        int address = (int)addr + 8 + bit_shift;
        length += signed_to_base(buf + length, bufsize, address, 16, 1);
    }

    return length;
}
