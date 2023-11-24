#![no_std]

/// This library defines types (structs, enums, ...) related to syscalls,
/// that need to be shared between the kernel and the uapi.
///
/// Most important enum is `Syscall`, which defines the list of available
/// syscalls and sets their identifier/number.
#[repr(C)]
#[derive(Debug)]
pub enum Syscall {
    Exit,
    GetProcessHandle,
    Yield,
    Sleep,
    Start,
    Map,
    Unmap,
    SHMSetCredential,
    SendIPC,
    SendSignal,
    WaitForEvent,
    ManageCPUSleep,

    #[cfg(debug_assertions)]
    Log,
}

/// Implementing fallible conversion from `u32` to `Syscall`
/// serves as the basis for the syscall dispatcher in the crate
/// `gate`.
impl TryFrom<u8> for Syscall {
    type Error = DispatchError;
    fn try_from(v: u8) -> Result<Syscall, Self::Error> {
        match v {
            0 => Ok(Syscall::Exit),
            1 => Ok(Syscall::GetProcessHandle),
            2 => Ok(Syscall::Yield),
            3 => Ok(Syscall::Sleep),
            4 => Ok(Syscall::Start),
            5 => Ok(Syscall::Map),
            6 => Ok(Syscall::Unmap),
            7 => Ok(Syscall::SHMSetCredential),
            8 => Ok(Syscall::SendIPC),
            9 => Ok(Syscall::SendSignal),
            10 => Ok(Syscall::WaitForEvent),
            11 => Ok(Syscall::ManageCPUSleep),

            #[cfg(debug_assertions)]
            12 => Ok(Syscall::Log),

            _ => Err(DispatchError::IllegalValue),
        }
    }
}

/// Possible errors when converting from a u32 to a syscall number
pub enum DispatchError {
    IllegalValue,
    InsufficientCapabilities,
}

/// Sentry syscall return values
#[repr(C)]
#[derive(Debug, PartialEq)]
pub enum Status {
    Ok,
    Invalid,
    Denied,
    NoEntity,
    Busy,
    AlreadyMapped,
    TimeOut,
    Critical,
}

impl From<u32> for Status {
    fn from(status_int: u32) -> Status {
        match status_int {
            0 => Status::Ok,
            1 => Status::Invalid,
            2 => Status::Denied,
            3 => Status::NoEntity,
            4 => Status::Busy,
            5 => Status::AlreadyMapped,
            6 => Status::TimeOut,
            7 => Status::Critical,
            _ => todo!(),
        }
    }
}

/// A process label is a development-time fixed identifier that can be used hardcoded
///  in the source code. This can be used in order to get back remote process effective
/// identifier from label at any time in order to communicate
#[repr(C)]
pub enum ProcessLabel {
    Label0,
}

impl From<ProcessLabel> for u32 {
    fn from(pl: ProcessLabel) -> u32 {
        match pl {
            ProcessLabel::Label0 => 0,
        }
    }
}

/// List of Sentry resource types
///
/// Multiple opaque are used in userspace. This opaques are namespaced and
/// manipulated at kernel level to ensure unicity, identification, etc.
#[repr(C)]
pub enum ResourceType {
    PID = 1,
    Device = 2,
    SHM = 4,
    DMA = 8,
}

/// Any of the above resource can be acceded only through a handle. This handle is unique and
/// must be used as an opaque in userspace implementation.
/// This allows fully-portable and reusable implementation of tasks, libraries, drivers so
/// avoiding any hard-coded content. All SoC specific content is manipulated by device-trees
/// exclusively, generating the overall handles.
#[cfg(not(feature = "cbindgen"))]
pub type ResourceHandle = u32;
#[cfg(feature = "cbindgen")]
#[repr(C)]
pub struct ResourceHandle {
    /// cbindgen:bitfield=4
    /// forged from dtsi, matches resource type
    resource_type: u32,
    /// cbindgen:bitfield=12
    /// task namespace, matches task label, forged at build time
    task_ns: u32,
    /// cbindgen:bitfield=12
    /// forged from dtsi if not a process, or at process startup
    resource_id: u32,
}

#[repr(C)]
pub enum SHMPermission {
    /// allows target process to map the SHM. No read nor write though
    Map,

    /// allows target process to read the mapped SHM. Requires MAP
    Read,

    /// allows target process to write shared memory. Requires MAP
    Write,

    /// allows target process to transfer SHM to another, pre-allowed, process
    Transfer,
}

impl From<SHMPermission> for u32 {
    fn from(shm_perm: SHMPermission) -> u32 {
        match shm_perm {
            SHMPermission::Map => 0x1,
            SHMPermission::Read => 0x2,
            SHMPermission::Write => 0x4,
            SHMPermission::Transfer => 0x8,
        }
    }
}

/// Events tasks can listen on
#[repr(C)]
pub enum Signal {
    /// Abort signal
    Abort,

    /// Timer (from alarm)
    Alarm,

    /// Bus error (bad memory access, memory required)
    Bus,

    /// Continue if previously stopped
    Cont,

    /// Illegal instruction. Can be also used for upper provtocols
    Ill,

    /// I/O now ready
    Io,

    /// Broken pipe
    Pipe,

    /// Event pollable
    Poll,

    /// Termination signal
    Term,

    /// Trace/bp signal (debug usage only)
    Trap,

    /// 1st user-defined signal
    Usr1,

    /// 2nd user-defined signal
    Usr2,
}

pub type ProcessID = u32;

#[repr(C)]
pub enum SleepDuration {
    D1ms,
    D2ms,
    D5ms,
    D10ms,
    D20ms,
    D50ms,
    ArbitraryMs(u32),
}

impl From<SleepDuration> for u32 {
    fn from(duration: SleepDuration) -> u32 {
        match duration {
            SleepDuration::D1ms => 1,
            SleepDuration::D2ms => 2,
            SleepDuration::D5ms => 5,
            SleepDuration::D10ms => 10,
            SleepDuration::D20ms => 20,
            SleepDuration::D50ms => 50,
            SleepDuration::ArbitraryMs(v) => v,
        }
    }
}

#[repr(C)]
pub enum SleepMode {
    Shallow,
    Deep,
}

impl From<SleepMode> for u32 {
    fn from(mode: SleepMode) -> u32 {
        match mode {
            SleepMode::Shallow => 0,
            SleepMode::Deep => 1,
        }
    }
}

#[repr(C)]
pub enum CPUSleep {
    WaitForInterrupt,
    WaitForEvent,
    ForbidSleep,
    AllowSleep,
}

impl From<CPUSleep> for u32 {
    fn from(mode: CPUSleep) -> u32 {
        match mode {
            CPUSleep::WaitForInterrupt => 0,
            CPUSleep::WaitForEvent => 1,
            CPUSleep::ForbidSleep => 2,
            CPUSleep::AllowSleep => 3,
        }
    }
}

impl TryFrom<u32> for CPUSleep {
    type Error = DispatchError;
    fn try_from(mode: u32) -> Result<CPUSleep, Self::Error> {
        match mode {
            0 => Ok(CPUSleep::WaitForInterrupt),
            1 => Ok(CPUSleep::WaitForEvent),
            2 => Ok(CPUSleep::ForbidSleep),
            3 => Ok(CPUSleep::AllowSleep),
            _ => Err(DispatchError::IllegalValue),
        }
    }
}