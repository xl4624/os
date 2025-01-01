#include <stdint.h>

extern "C" struct interrupt_frame;

enum ISR : uint8_t {
    DivideByZero = 0,
    Debug = 1,
    NonMaskableInterrupt = 2,
    Breakpoint = 3,
    Overflow = 4,
    BoundRangeExceeded = 5,
    InvalidOpcode = 6,
    DeviceNotAvailable = 7,
    DoubleFault = 8,
    CoprocessorSegmentOverrun = 9,
    InvalidTSS = 10,
    SegmentNotPresent = 11,
    StackFault = 12,
    GeneralProtection = 13,
    PageFault = 14,
    Reserved15 = 15,
    X87FloatingPoint = 16,
    AlignmentCheck = 17,
    MachineCheck = 18,
    SIMDFloatingPoint = 19,
    Virtualization = 20,
    ControlProtection = 21,
    Reserved22 = 22,
    Reserved23 = 23,
    Reserved24 = 24,
    Reserved25 = 25,
    Reserved26 = 26,
    Reserved27 = 27,
    HypervisionInjection = 28,
    VMMCommunication = 29,
    SecurityException = 30,
    Reserved31 = 31
};

enum IRQ : uint8_t {
    Timer = 0,
    Keyboard = 1,
    Cascade = 2,
    COM2 = 3,
    COM1 = 4,
    LPT2 = 5,
    FloppyDisk = 6,
    LPT1 = 7,
    CMOS_RTC = 8,
    Free1 = 9,
    Free2 = 10,
    Free3 = 11,
    PS2Mouse = 12,
    FPU = 13,
    PrimaryATA = 14,
    SecondaryATA = 15,
};

using handler_t = void (*)(interrupt_frame *);

extern "C" void interrupt_init();

void exception_register_handler(ISR isr, handler_t handler);
void interrupt_register_handler(IRQ irq, handler_t handler);

void interrupt_enable();
void interrupt_disable();
