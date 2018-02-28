#include <pcut/pcut.h>
#include "../hw_struct/regs.h"

PCUT_INIT

static struct {
	uint32_t field32;
	uint16_t field16;
	uint8_t field8;
} regs[1];

#define REG_8_FLAG    field8,  8,  FLAG,  3
#define REG_8_RANGE   field8,  8, RANGE,  6, 2
#define REG_8_FIELD   field8,  8, FIELD
#define REG_16_FLAG  field16, 16,  FLAG,  8
#define REG_16_RANGE field16, 16, RANGE, 11, 4
#define REG_16_FIELD field16, 16, FIELD
#define REG_32_FLAG  field32, 32,  FLAG, 16
#define REG_32_RANGE field32, 32, RANGE, 23, 8
#define REG_32_FIELD field32, 32, FIELD

#define RESET memset(regs, 0, sizeof(regs[0]))
#define EQ(exp, act) PCUT_ASSERT_INT_EQUALS((exp), (act))

PCUT_TEST(ops_8_field) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_8_FIELD));

	XHCI_REG_WR(regs, REG_8_FIELD, 0x55);
	EQ(0x55, XHCI_REG_RD(regs, REG_8_FIELD));
	EQ(0x55, regs->field8);

	RESET;
	XHCI_REG_SET(regs, REG_8_FIELD, 0x55);
	EQ(0x55, XHCI_REG_RD(regs, REG_8_FIELD));
	EQ(0x55, regs->field8);

	XHCI_REG_CLR(regs, REG_8_FIELD, 0x5);
	EQ(0x50, XHCI_REG_RD(regs, REG_8_FIELD));
	EQ(0x50, regs->field8);
}

PCUT_TEST(ops_8_range) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_8_RANGE));

	XHCI_REG_WR(regs, REG_8_RANGE, 0x55);
	EQ(0x15, XHCI_REG_RD(regs, REG_8_RANGE));
	EQ(0x54, regs->field8);

	XHCI_REG_SET(regs, REG_8_RANGE, 0x2);
	EQ(0x17, XHCI_REG_RD(regs, REG_8_RANGE));
	EQ(0x5c, regs->field8);

	XHCI_REG_CLR(regs, REG_8_RANGE, 0x2);
	EQ(0x15, XHCI_REG_RD(regs, REG_8_RANGE));
	EQ(0x54, regs->field8);
}

PCUT_TEST(ops_8_flag) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_8_FLAG));

	XHCI_REG_WR(regs, REG_8_FLAG, 1);
	EQ(1, XHCI_REG_RD(regs, REG_8_FLAG));
	EQ(8, regs->field8);

	RESET;
	XHCI_REG_SET(regs, REG_8_FLAG, 1);
	EQ(1, XHCI_REG_RD(regs, REG_8_FLAG));
	EQ(8, regs->field8);

	XHCI_REG_CLR(regs, REG_8_FLAG, 1);
	EQ(0, XHCI_REG_RD(regs, REG_8_FLAG));
	EQ(0, regs->field8);
}

PCUT_TEST(ops_16_field) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_16_FIELD));

	XHCI_REG_WR(regs, REG_16_FIELD, 0x5555);
	EQ(0x5555, XHCI_REG_RD(regs, REG_16_FIELD));
	EQ(0x5555, xhci2host(16, regs->field16));

	XHCI_REG_SET(regs, REG_16_FIELD, 0x00aa);
	EQ(0x55ff, XHCI_REG_RD(regs, REG_16_FIELD));
	EQ(0x55ff, xhci2host(16, regs->field16));

	XHCI_REG_CLR(regs, REG_16_FIELD, 0x055a);
	EQ(0x50a5, XHCI_REG_RD(regs, REG_16_FIELD));
	EQ(0x50a5, xhci2host(16, regs->field16));
}

PCUT_TEST(ops_16_range) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_16_RANGE));

	XHCI_REG_WR(regs, REG_16_RANGE, 0x5a5a);
	EQ(0x5a, XHCI_REG_RD(regs, REG_16_RANGE));
	EQ(0x05a0, xhci2host(16, regs->field16));

	XHCI_REG_SET(regs, REG_16_RANGE, 0xa5);
	EQ(0xff, XHCI_REG_RD(regs, REG_16_RANGE));
	EQ(0x0ff0, xhci2host(16, regs->field16));

	XHCI_REG_CLR(regs, REG_16_RANGE, 0x5a);
	EQ(0xa5, XHCI_REG_RD(regs, REG_16_RANGE));
	EQ(0x0a50, xhci2host(16, regs->field16));
}

PCUT_TEST(ops_16_flag) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_16_FLAG));

	XHCI_REG_WR(regs, REG_16_FLAG, 1);
	EQ(1, XHCI_REG_RD(regs, REG_16_FLAG));
	EQ(0x100, xhci2host(16, regs->field16));

	RESET;
	XHCI_REG_SET(regs, REG_16_FLAG, 1);
	EQ(1, XHCI_REG_RD(regs, REG_16_FLAG));
	EQ(0x100, xhci2host(16, regs->field16));

	XHCI_REG_CLR(regs, REG_16_FLAG, 1);
	EQ(0, XHCI_REG_RD(regs, REG_16_FLAG));
	EQ(0, xhci2host(16, regs->field16));
}

PCUT_TEST(ops_32_field) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_32_FIELD));

	XHCI_REG_WR(regs, REG_32_FIELD, 0xffaa5500);
	EQ(0xffaa5500, XHCI_REG_RD(regs, REG_32_FIELD));
	EQ(0xffaa5500, xhci2host(32, regs->field32));

	XHCI_REG_SET(regs, REG_32_FIELD, 0x0055aa00);
	EQ(0xffffff00, XHCI_REG_RD(regs, REG_32_FIELD));
	EQ(0xffffff00, xhci2host(32, regs->field32));

	XHCI_REG_CLR(regs, REG_32_FIELD, 0x00aa55ff);
	EQ(0xff55aa00, XHCI_REG_RD(regs, REG_32_FIELD));
	EQ(0xff55aa00, xhci2host(32, regs->field32));
}

PCUT_TEST(ops_32_range) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_32_RANGE));

	XHCI_REG_WR(regs, REG_32_RANGE, 0xff5a0);
	EQ(0xf5a0, XHCI_REG_RD(regs, REG_32_RANGE));
	EQ(0x00f5a000, xhci2host(32, regs->field32));

	XHCI_REG_SET(regs, REG_32_RANGE, 0xffa50);
	EQ(0xfff0, XHCI_REG_RD(regs, REG_32_RANGE));
	EQ(0x00fff000, xhci2host(32, regs->field32));

	XHCI_REG_CLR(regs, REG_32_RANGE, 0xf05af);
	EQ(0xfa50, XHCI_REG_RD(regs, REG_32_RANGE));
	EQ(0x00fa5000, xhci2host(32, regs->field32));
}

PCUT_TEST(ops_32_flag) {
	RESET;
	EQ(0, XHCI_REG_RD(regs, REG_32_FLAG));

	XHCI_REG_WR(regs, REG_32_FLAG, 1);
	EQ(1, XHCI_REG_RD(regs, REG_32_FLAG));
	EQ(0x10000, xhci2host(32, regs->field32));

	RESET;
	XHCI_REG_SET(regs, REG_32_FLAG, 1);
	EQ(1, XHCI_REG_RD(regs, REG_32_FLAG));
	EQ(0x10000, xhci2host(32, regs->field32));

	XHCI_REG_CLR(regs, REG_32_FLAG, 1);
	EQ(0, XHCI_REG_RD(regs, REG_32_FLAG));
	EQ(0, xhci2host(32, regs->field32));
}
PCUT_MAIN();
