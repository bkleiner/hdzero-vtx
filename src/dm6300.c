#include "dm6300.h"

#include "driver/spi.h"

#include "debug.h"
#include "eeprom.h"

dm6300_efuse_t efuse;

uint32_t freq_cnt = 0;
uint32_t freq_num[FREQ_MAX_EXT] = {0};

uint8_t power_table[FREQ_MAX_EXT][POWER_MAX];

const uint32_t freq_tab[FREQ_MAX_EXT] = {113200, 113900, 114700, 115400, 116100, 116780, 117560, 118280, 115200, 116000};
const uint32_t freq_reg_tab[3][FREQ_MAX_EXT] = {
    {0x3867, 0x379D, 0x3924, 0x3982, 0x39E1, 0x3A3F, 0x3A9E, 0x3AFC, 0x3840, 0x38A4},
    {0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x96, 0x97},
    {0xB00000, 0x9D5555, 0x8AAAAB, 0x780000, 0x655555, 0x52AAAB, 0x400000, 0x2D5555, 0x000000, 0x155555},
};

#ifdef VTX_L
const uint16_t power_reg_tab[4] = {0x204, 0x11F, 0x21F, 0x31F};
#else
const uint16_t power_reg_tab[2] = {0x21F, 0x41F};
#endif

void dm6300_read_efuse() {
    debugf("dm6300 efuse size %d\r\n", sizeof(dm6300_efuse_t));

    // EFUSE_RST = 1;
    spi_write(0x6, 0xFF0, 0x00000019);
    spi_write(0x3, 0x0E0, 0x00000001);

    spi_write(0x6, 0xFF0, 0x00000018);

    uint8_t val = 0;
    uint32_t dat = 0;
    uint8_t efuse_busy = 1;
    for (uint8_t j = 0; j < EFUSE_NUM; j++) {
        debugf("dm6300 reading efuse %d\r\n", j);

        for (uint8_t i = 0; i < EFUSE_SIZE; i++) {
            spi_write(0x3, 0x7D0, (j << 11) | (i << 4) | 0x1);

            while (efuse_busy) {
                spi_read(0x3, 0x7D4, &dat);
                efuse_busy = dat & 1;
            }
            efuse_busy = 1;

            spi_read(0x3, 0x7D8, &dat);
            val = dat & 0xFF;
            efuse.raw[j][i] = val;
        }
#ifdef DEBUG_EFUSE
        debug_hex(efuse.raw[j], EFUSE_SIZE);
        debugf("\r\n");
#endif
    }

    // EFUSE_CFG = 0;
    spi_write(0x3, 0x7D0, 0x00000000);
    // EFUSE_RST = 0;
    spi_write(0x6, 0xFF0, 0x00000019);
    spi_write(0x3, 0x0E0, 0x00000000);

    spi_write(0x6, 0xF14, efuse.macros.m1.bandgap);
    dat = ((efuse.macros.m1.ical & 0x1F) << 3) | (efuse.macros.m1.rcal & 0x7);
    spi_write(0x6, 0xF18, dat);

    spi_write(0x6, 0xFF0, 0x00000018);
}

const spi_reg_value_t dm6300_init0_regs[] = {
    {0x6, 0xFFC, 0x00000000},
    {0x6, 0xFFC, 0x00000001},
    {0x6, 0x7FC, 0x00000000},
    {0x6, 0xF1C, 0x00000001},
    {0x6, 0xF20, 0x0000FCD0},
    {0x6, 0xF04, 0x00004741},
    {0x6, 0xF08, 0x00000083},
    {0x6, 0xF08, 0x000000C3},
    {0x6, 0xF40, 0x00000003},
    {0x6, 0xF40, 0x00000001},
    {0x6, 0xFF0, 0x00000018},
    {0x6, 0xFFC, 0x00000000},
    {0x6, 0xFFC, 0x00000001},
};

const spi_reg_value_t dm6300_init1_regs[] = {
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2B0, 0x00077777},
    {0x3, 0x030, 0x00000013},
    {0x3, 0x034, 0x00000013},
    {0x3, 0x038, 0x00000370},
    {0x3, 0x03C, 0x00000410},
    {0x3, 0x040, 0x00000000},
    {0x3, 0x044, 0x0D640735},
    {0x3, 0x048, 0x01017F03},
    {0x3, 0x04C, 0x021288A2},
    {0x3, 0x050, 0x00FFCF33},
    {0x3, 0x054, 0x1F3C3440},
    {0x3, 0x028, 0x00008008},
    {0x3, 0x01C, 0x0000FFF6},
    {0x3, 0x018, 0x00000001},
    {0x3, 0x018, 0x00000000}};

const spi_reg_value_t dm6300_init2_regs_bw20[] = {
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2AC, 0x00000300},
    {0x3, 0x2B0, 0x00007007},
    {0x3, 0x230, 0x00000000},
    {0x3, 0x234, 0x10000000},
    {0x3, 0x238, 0x000000BF},
    {0x3, 0x23C, 0x55530610}, // 0x15840410  0x15530610
    {0x3, 0x240, 0x3FFC0047}, // 0x0001C047  0x3FFC0047
    {0x3, 0x244, 0x00188A13}, // 0x00188A17
    {0x3, 0x248, 0x00000000},
    {0x3, 0x24C, 0x0A121707}, // 0x08021707
    {0x3, 0x250, 0x017F0001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x228, 0x0000807A},
    {0x3, 0x220, 0x00002FA8}, // 0x00002666
    {0x3, 0x21C, 0x00000002},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x218, 0x00000001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x21C, 0x00000003},
    {0x3, 0x218, 0x00000001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x244, 0x00188A17},
    {0x3, 0x204, 0x00000032}, // 0x0000004c
    {0x3, 0x208, 0x00000000}, // 0x00666666
    {0x3, 0x200, 0x00000000},
    {0x3, 0x200, 0x00000003},
    {0x3, 0x240, 0x00030041}, // 0x0003C045
    {0x3, 0x248, 0x00000404},
    {0x3, 0x250, 0x0F7F0001},
    {0x3, 0x254, 0x00007813}, // 0x00007813
    {0x3, 0x258, 0x00010003},
};
const spi_reg_value_t dm6300_init2_regs_bw27[] = {
    // 02_BBPLL_3456
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2AC, 0x00000300},
    {0x3, 0x2B0, 0x00077777},
    {0x3, 0x230, 0x00000000},
    {0x3, 0x234, 0x10000000},
    {0x3, 0x238, 0x000000BF},
    {0x3, 0x23C, 0x17530610}, // 0x15840410  0x15530610
    {0x3, 0x240, 0x3FFC0047}, // 0x0001C047  0x3FFC0047
    {0x3, 0x244, 0x00188A13}, // 0x00188A17
    {0x3, 0x248, 0x00000000},
    {0x3, 0x24C, 0x0A161707}, // 0x08021707
    {0x3, 0x250, 0x017F0001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x228, 0x00008083},
    {0x3, 0x220, 0x00002E0E}, // 0x00002666
    {0x3, 0x21C, 0x00000002},
    {0x3, 0x218, 0x00000001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x21C, 0x00000003},
    {0x3, 0x218, 0x00000001},
    {0x3, 0x218, 0x00000000},
    {0x3, 0x244, 0x00188A17},
    {0x3, 0x204, 0x0000002D}, // 0x0000004c
    {0x3, 0x208, 0x00000000}, // 0x00666666
    {0x3, 0x200, 0x00000000},
    {0x3, 0x200, 0x00000003},
    {0x3, 0x240, 0x00030041}, // 0x0003C045
    {0x3, 0x248, 0x00000404},
    {0x3, 0x250, 0x0F7F0001},
    {0x3, 0x254, 0x00007813}, // 0x00007813
    {0x3, 0x258, 0x00010002},
};

spi_reg_value_t dm6300_set_channel_regs[] = {
    // 03_RFPLL_CA1_TX_10G
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x2B0, 0x00077777},
    {0x3, 0x030, 0x00000013},
    {0x3, 0x034, 0x00000013},
    {0x3, 0x038, 0x00000370},
    {0x3, 0x03C, 0x00000410},
    {0x3, 0x040, 0x00000000},
    {0x3, 0x044, 0x0D640735},
    {0x3, 0x048, 0x01017F03},
    {0x3, 0x04C, 0x021288A2},
    {0x3, 0x050, 0x00FFCF33},
    {0x3, 0x054, 0x1F3C3440},
    // {0x3, 0x028, 0x00008030},
    {0x3, 0x028, 0x00000000}, // [12] // 0x00008000 + (freq_cnt & 0xFF)
    // {0x3, 0x020, 0x00003746},
    {0x3, 0x020, 0x00000000}, // [13] // freq_num[ch]}, // freq_reg_tab[0][ch] WAIT(1},
    {0x3, 0x01C, 0x00000002},
    {0x3, 0x018, 0x00000001},
    {0x3, 0x018, 0x00000000},
    // {0x3, 0x028, 0x00008030},
    {0x3, 0x028, 0x00000000}, // [17] // 0x00008000 + (freq_cnt & 0xFF)},
    // {0x3, 0x020, 0x00003746},
    {0x3, 0x020, 0x00000000}, // [18] // freq_num[ch]}, // freq_reg_tab[0][ch]
    {0x3, 0x01C, 0x00000003},
    {0x3, 0x018, 0x00000001},
    {0x3, 0x018, 0x00000000},
    {0x3, 0x050, 0x00FFCFB3},
    // {0x3, 0x004, 0x00000093},
    // {0x3, 0x008, 0x00CAAAAB},
    {0x3, 0x004, 0x00000000}, // [23] // freq_reg_tab[1][ch]},
    {0x3, 0x008, 0x00000000}, // [24] // freq_reg_tab[2][ch]},
    {0x3, 0x000, 0x00000000},
    {0x3, 0x000, 0x00000003},
    {0x3, 0x050, 0x000333B3},
    {0x3, 0x040, 0x07070002},
    {0x3, 0x030, 0x00000010},
};

void dm6300_set_channel(uint8_t ch) {
    if (ch >= FREQ_MAX_EXT) {
        ch = 0;
    }

    dm6300_set_channel_regs[12].dat = 0x00008000 + (freq_cnt & 0xFF);
    dm6300_set_channel_regs[13].dat = freq_num[ch]; // freq_reg_tab[0][ch] WAIT(1);

    dm6300_set_channel_regs[17].dat = 0x00008000 + (freq_cnt & 0xFF);
    dm6300_set_channel_regs[18].dat = freq_num[ch]; // freq_reg_tab[0][ch]

    dm6300_set_channel_regs[23].dat = freq_reg_tab[1][ch];
    dm6300_set_channel_regs[24].dat = freq_reg_tab[2][ch];

    SPI_WRITE_REG_MAP(dm6300_set_channel_regs);
}

const spi_reg_value_t dm6300_init4_regs[] = {
    // 04_TX_CA1_RF
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x31C, 0x00000000}, // 0x00000030
    {0x3, 0x300, 0xC000281B},
    {0x3, 0x304, 0x0CC00006},
    {0x3, 0x308, 0x00000000},
    {0x3, 0x30C, 0x00000000},
    {0x3, 0x310, 0xFFFFFF14},
    {0x3, 0x314, 0x4B7FFFE0},
    {0x3, 0x318, 0xFFFFFFFF},
    {0x3, 0x320, 0x00008000},
    {0x3, 0x324, 0x0000004C},
    {0x3, 0x328, 0x00000000},
    {0x3, 0x32C, 0x00000000},
    {0x3, 0x330, 0x00000000}, // remove init DC high
    {0x3, 0x334, 0x00000000},
    {0x3, 0x31C, 0x00000011},
};

const spi_reg_value_t dm6300_init5_regs[] = {
    // 05_tx_cal_DAC_BBF
    {0x6, 0xFF0, 0x00000019},
    {0x3, 0x194, 0x0001FFFF},
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0x778, 0x80000A80}, // 0x80000A80 0x00400680
    {0x3, 0x77C, 0x0D000000}, // 0x0D000000 0x0D800680
    {0x3, 0x780, 0x80000A80}, // 0x80000A80 0x00400680
    {0x3, 0x784, 0x0D000000}, // 0x0D000000 0x0D800680
    {0x3, 0x788, 0x0FFFFFFF},
    {0x3, 0x78C, 0x00300FD6}, // 0x00300FD6 0x80080FD5
    {0x3, 0x384, 0x00075075},
    {0x3, 0x38C, 0x00075075},
    {0x3, 0x390, 0x042F5444},
    {0x3, 0x394, 0x2E788108},
    {0x3, 0x398, 0x2E788128},
    {0x3, 0x39C, 0x2C80C0C8},
    {0x3, 0x3A0, 0x00006868},
    {0x3, 0x3A4, 0x04150404},
    {0x3, 0x3A8, 0x00000000},
    {0x3, 0x3AC, 0x00000000},
    {0x3, 0x3B0, 0x00000000},
    {0x3, 0x3D8, 0x0000000A},
    {0x3, 0x380, 0x075F8000},
    {0x3, 0x388, 0x075F8000},
};

spi_reg_value_t dm6300_init6_regs[] = {
    {0x6, 0xFF0, 0x00000019},
    {0x3, 0x0E4, 0x00000002},
    {0x3, 0x0E8, 0x0000000D},
    {0x3, 0x08c, 0x00000000},

    {0x6, 0xFF0, 0x00000018},
    {0x3, 0xd04, 0x020019D9}, // 0x0x020000ED
    {0x3, 0xd08, 0x00000000},
    {0x3, 0xd1c, 0x00000000}, // remove init DC high
    {0x3, 0xd00, 0x00000003},

    {0x6, 0xFF0, 0x00000019},

    {0x3, 0x080, 0x00000000}, // [10]

    // if (sel)
    //     spi_write(0x3, 0x080, 0x16318C0C);
    // else
    //     spi_write(0x3, 0x080, 0x1084210C);

    {0x3, 0x020, 0x0000000C},
    {0x3, 0x018, 0x04F16040},
    {0x3, 0x088, 0x00000085},

    {0x6, 0xFF0, 0x00000018},
    {0x3, 0xD08, 0x03200300},
    {0x3, 0xD0C, 0x00000000},
};

void dm6300_init6() {
    dm6300_init6_regs[10].dat = DM6300_BW ? 0x16318C0C : 0x1084210C;
    SPI_WRITE_REG_MAP(dm6300_init6_regs);
}

const spi_reg_value_t dm6300_init7_regs_bw20[] = {
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0xC00, 0x005B0047},
    {0x3, 0xC04, 0xFFBB0017},
    {0x3, 0xC08, 0xFFB2FF8D},
    {0x3, 0xC0C, 0x006B0011},
    {0x3, 0xC10, 0x003E0080},
    {0x3, 0xC14, 0xFF78FFCE},
    {0x3, 0xC18, 0xFFD4FF78},
    {0x3, 0xC1C, 0x00A30052},
    {0x3, 0xC20, 0x0018008C},
    {0x3, 0xC24, 0xFF43FF8C},
    {0x3, 0xC28, 0x0001FF71},
    {0x3, 0xC2C, 0x00D80099},
    {0x3, 0xC30, 0xFFE2008E},
    {0x3, 0xC34, 0xFF0CFF3D},
    {0x3, 0xC38, 0x0042FF77},
    {0x3, 0xC3C, 0x011200F2},
    {0x3, 0xC40, 0xFF910081},
    {0x3, 0xC44, 0xFECDFED5},
    {0x3, 0xC48, 0x00A9FF8D},
    {0x3, 0xC4C, 0x01580170},
    {0x3, 0xC50, 0xFF0C005D},
    {0x3, 0xC54, 0xFE7AFE38},
    {0x3, 0xC58, 0x015DFFC4},
    {0x3, 0xC5C, 0x01C20240},
    {0x3, 0xC60, 0xFE060007},
    {0x3, 0xC64, 0xFDE4FD0A},
    {0x3, 0xC68, 0x03050054},
    {0x3, 0xC6C, 0x02BD0434},
    {0x3, 0xC70, 0xFAB7FEEF},
    {0x3, 0xC74, 0xFB99F8D8},
    {0x3, 0xC78, 0x0E9D0376},
    {0x3, 0xC7C, 0x20271979},
    {0x3, 0xC80, 0x19792027},
    {0x3, 0xC84, 0x03760E9D},
    {0x3, 0xC88, 0xF8D8FB99},
    {0x3, 0xC8C, 0xFEEFFAB7},
    {0x3, 0xC90, 0x043402BD},
    {0x3, 0xC94, 0x00540305},
    {0x3, 0xC98, 0xFD0AFDE4},
    {0x3, 0xC9C, 0x0007FE06},
    {0x3, 0xCA0, 0x024001C2},
    {0x3, 0xCA4, 0xFFC4015D},
    {0x3, 0xCA8, 0xFE38FE7A},
    {0x3, 0xCAC, 0x005DFF0C},
    {0x3, 0xCB0, 0x01700158},
    {0x3, 0xCB4, 0xFF8D00A9},
    {0x3, 0xCB8, 0xFED5FECD},
    {0x3, 0xCBC, 0x0081FF91},
    {0x3, 0xCC0, 0x00F20112},
    {0x3, 0xCC4, 0xFF770042},
    {0x3, 0xCC8, 0xFF3DFF0C},
    {0x3, 0xCCC, 0x008EFFE2},
    {0x3, 0xCD0, 0x009900D8},
    {0x3, 0xCD4, 0xFF710001},
    {0x3, 0xCD8, 0xFF8CFF43},
    {0x3, 0xCDC, 0x008C0018},
    {0x3, 0xCE0, 0x005200A3},
    {0x3, 0xCE4, 0xFF78FFD4},
    {0x3, 0xCE8, 0xFFCEFF78},
    {0x3, 0xCEC, 0x0080003E},
    {0x3, 0xCF0, 0x0011006B},
    {0x3, 0xCF4, 0xFF8DFFB2},
    {0x3, 0xCF8, 0x0017FFBB},
    {0x3, 0xCFC, 0x0047005B},
};
const spi_reg_value_t dm6300_init7_regs_bw27[] = {
    // 07_fir_128stap
    {0x6, 0xFF0, 0x00000018},
    {0x3, 0xC00, 0x004E0041},
    {0x3, 0xC04, 0x0053005D},
    {0x3, 0xC08, 0xFFF7002D},
    {0x3, 0xC0C, 0xFFB1FFC7},
    {0x3, 0xC10, 0xFFF0FFC1},
    {0x3, 0xC14, 0x00450025},
    {0x3, 0xC18, 0x0009003B},
    {0x3, 0xC1C, 0xFF98FFC6},
    {0x3, 0xC20, 0xFFD2FF9A},
    {0x3, 0xC24, 0x00670024},
    {0x3, 0xC28, 0x00380071},
    {0x3, 0xC2C, 0xFF79FFD4},
    {0x3, 0xC30, 0xFF94FF5C},
    {0x3, 0xC34, 0x0085000B},
    {0x3, 0xC38, 0x008A00BC},
    {0x3, 0xC3C, 0xFF620000},
    {0x3, 0xC40, 0xFF2DFF09},
    {0x3, 0xC44, 0x0093FFCA},
    {0x3, 0xC48, 0x010D011B},
    {0x3, 0xC4C, 0xFF640060},
    {0x3, 0xC50, 0xFE88FE9F},
    {0x3, 0xC54, 0x007DFF42},
    {0x3, 0xC58, 0x01E70195},
    {0x3, 0xC5C, 0xFF9C0127},
    {0x3, 0xC60, 0xFD5BFE0B},
    {0x3, 0xC64, 0x000FFE15},
    {0x3, 0xC68, 0x03B9025D},
    {0x3, 0xC6C, 0x007B0323},
    {0x3, 0xC70, 0xF9E2FCC5},
    {0x3, 0xC74, 0xFDD2F9D3},
    {0x3, 0xC78, 0x0F7905A3},
    {0x3, 0xC7C, 0x1DE41880},
    {0x3, 0xC80, 0x18801DE4},
    {0x3, 0xC84, 0x05A30F79},
    {0x3, 0xC88, 0xF9D3FDD2},
    {0x3, 0xC8C, 0xFCC5F9E2},
    {0x3, 0xC90, 0x0323007B},
    {0x3, 0xC94, 0x025D03B9},
    {0x3, 0xC98, 0xFE15000F},
    {0x3, 0xC9C, 0xFE0BFD5B},
    {0x3, 0xCA0, 0x0127FF9C},
    {0x3, 0xCA4, 0x019501E7},
    {0x3, 0xCA8, 0xFF42007D},
    {0x3, 0xCAC, 0xFE9FFE88},
    {0x3, 0xCB0, 0x0060FF64},
    {0x3, 0xCB4, 0x011B010D},
    {0x3, 0xCB8, 0xFFCA0093},
    {0x3, 0xCBC, 0xFF09FF2D},
    {0x3, 0xCC0, 0x0000FF62},
    {0x3, 0xCC4, 0x00BC008A},
    {0x3, 0xCC8, 0x000B0085},
    {0x3, 0xCCC, 0xFF5CFF94},
    {0x3, 0xCD0, 0xFFD4FF79},
    {0x3, 0xCD4, 0x00710038},
    {0x3, 0xCD8, 0x00240067},
    {0x3, 0xCDC, 0xFF9AFFD2},
    {0x3, 0xCE0, 0xFFC6FF98},
    {0x3, 0xCE4, 0x003B0009},
    {0x3, 0xCE8, 0x00250045},
    {0x3, 0xCEC, 0xFFC1FFF0},
    {0x3, 0xCF0, 0xFFC7FFB1},
    {0x3, 0xCF4, 0x002DFFF7},
    {0x3, 0xCF8, 0x005D0053},
    {0x3, 0xCFC, 0x0041004E},
};

void dm6300_configure_efuse() {
    debugf("dm6300 efuse bands\r\n");

    for (uint8_t i = 0; i < efuse.macros.m0.band_num; i++) {
        debugf("%d: start=%u, stop=%u\r\n", i, efuse.macros.m2[i].tx1.freq_start, efuse.macros.m2[i].tx1.freq_stop);

        if (efuse.macros.m2[i].tx1.freq_start < 5000 || efuse.macros.m2[i].tx1.freq_stop > 6000) {
            continue;
        }

        if ((efuse.macros.m0.efuse_ver[1] == '2') && (efuse.macros.m0.efuse_ver[3] == '3')) {
            efuse.macros.m2[i].tx1.dcoc_i = efuse.macros.m2[i].tx1.dcoc_i_dvm;
            efuse.macros.m2[i].tx1.dcoc_q = efuse.macros.m2[i].tx1.dcoc_q_dvm;
        } else {
            efuse.macros.m2[i].tx1.dcoc_i = efuse.macros.m2[i].tx1.dcoc_i;
            efuse.macros.m2[i].tx1.dcoc_q = efuse.macros.m2[i].tx1.dcoc_q;
        }

        if ((efuse.macros.m0.efuse_ver[1] == '2') && (efuse.macros.m0.efuse_ver[3] == '2')) {
            uint32_t ef_data = efuse.macros.m2[i].tx1.dcoc_i;
            ef_data = (ef_data & 0xFFFF8000) | ((((((ef_data & 0x00007f7f) + 0x00000202) >> 2) & 0x00001f1f)) | 0x00000080);
            efuse.macros.m2[i].tx1.dcoc_i = ef_data;

            ef_data = efuse.macros.m2[i].tx1.dcoc_q;
            ef_data = (ef_data & 0xFFFF8000) | ((((((ef_data & 0x00007f7f) + 0x00000202) >> 2) & 0x00001f1f)) | 0x00000080);
            efuse.macros.m2[i].tx1.dcoc_q = ef_data;
        }

        spi_write(0x3, 0xD08, efuse.macros.m2[i].tx1.iqmismatch);
        spi_write(0x3, 0x380, efuse.macros.m2[i].tx1.dcoc_i);
        spi_write(0x3, 0x388, efuse.macros.m2[i].tx1.dcoc_q);

        debugf("iqmismatch=%lx\r\n", efuse.macros.m2[i].tx1.iqmismatch);
        debugf("dcoc_i=%lx\r\n", efuse.macros.m2[i].tx1.dcoc_i);
        debugf("dcoc_q=%lx\r\n", efuse.macros.m2[i].tx1.dcoc_q);

        // TODO: settings from EEPROM
    }

    debugf("\r\n");
}

void dm6300_init_power_table() {
    debugf("dm6300 init power table\r\n");

    for (uint8_t j = 0; j < POWER_MAX; j++) {
        for (uint8_t i = 0; i < FREQ_MAX; i++) {
            power_table[i][j] = eeprom->power_table[i * POWER_MAX + j];
            if (j == 0) {
                power_table[i][j] += 0x0C;
            }
        }

        power_table[8][j] = eeprom->power_table[3 * POWER_MAX + j];
        power_table[9][j] = eeprom->power_table[4 * POWER_MAX + j];
        if (j == 0) { // 25mw +3dbm
            power_table[8][j] += 0x0C;
            power_table[9][j] += 0x0C;
        }
    }

    for (uint8_t i = 0; i < FREQ_MAX_EXT; i++) {
        debugf("power_table[%d] =", i);
        for (uint8_t j = 0; j < POWER_MAX; j++) {
            debugf(" %02X", power_table[i][j]);
        }
        debugf("\r\n");
    }
}

void dm6300_init(uint8_t ch) {
    debugf("dm6300 init\r\n");

    // 01_INIT
    SPI_WRITE_REG_MAP(dm6300_init0_regs);
    dm6300_read_efuse();
    SPI_WRITE_REG_MAP(dm6300_init1_regs);

    uint32_t dat = 0;
    spi_read(0x3, 0x02C, &dat);
    freq_cnt = dat & 0x3FFF;
    freq_cnt = 0x20000 / freq_cnt - 3;
    for (uint8_t i = 0; i < FREQ_MAX_EXT; i++) {
        freq_num[i] = freq_tab[i] * freq_cnt / 384;
    }

    // 02_BBPLL_3456
    if (DM6300_BW) {
        SPI_WRITE_REG_MAP(dm6300_init2_regs_bw20);
    } else {
        SPI_WRITE_REG_MAP(dm6300_init2_regs_bw27);
    }

    // 03_RFPLL_CA1_TX_10G
    dm6300_set_channel(ch);

    // 04_TX_CA1_RF
    SPI_WRITE_REG_MAP(dm6300_init4_regs);

    // 05_tx_cal_DAC_BBF
    SPI_WRITE_REG_MAP(dm6300_init5_regs);

    // 06_TX_CA1_RBDP_CMOS
    dm6300_init6();

    // 07_fir_128stap
    if (DM6300_BW) {
        SPI_WRITE_REG_MAP(dm6300_init7_regs_bw20);
    } else {
        SPI_WRITE_REG_MAP(dm6300_init7_regs_bw27);
    }

    dm6300_configure_efuse();

    dm6300_init_power_table();
}

void dm6300_set_power(uint8_t pwr, uint8_t ch, uint8_t offset) {
    if (ch >= FREQ_MAX_EXT) {
        ch = 0;
    }

    debugf("\r\ndm6300_set_power: %d ch: %d offset: %d", pwr, ch, offset);

    spi_write(0x6, 0xFF0, 0x00000018);

    if (pwr == POWER_MAX) {
        spi_write(0x3, 0x330, 0x21F);
        spi_write(0x3, 0xD1C, PIT_POWER);
    } else {
        spi_write(0x3, 0x330, power_reg_tab[pwr]);

        // TODO: OFFSET_25MW
        int16_t p = power_table[ch][pwr] + offset - 2;
        if (p > 255) {
            p = 255;
        } else if (p < 0) {
            p = 0;
        }
        spi_write(0x3, 0xD1C, (uint8_t)p);
    }
}