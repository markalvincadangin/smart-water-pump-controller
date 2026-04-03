/**
 * Gold Standard: usePumpData — Firebase data handling and control writers.
 * Mocks Firebase ref/onValue/set and auth to test hook behavior.
 */
import { renderHook, act, waitFor } from "@testing-library/react";
import { usePumpData } from "@/lib/usePumpData";

const mockUnsub = jest.fn();
const mockOnValue = jest.fn((..._args: any[]) => mockUnsub) as jest.Mock<any, any>;
const mockSet = jest.fn<Promise<void>, [unknown, unknown]>(() => Promise.resolve());
const mockUpdate = jest.fn<Promise<void>, [unknown, Record<string, unknown>]>(() => Promise.resolve());
const mockRef = jest.fn((db: unknown, p: string) => ({ _path: p }));
const mockOnAuthStateChanged = jest.fn((auth: unknown, cb: (u: unknown) => void) => {
  setTimeout(() => cb({ uid: "test-uid", email: "test@example.com" }), 0);
  return mockUnsub;
});

jest.mock("firebase/database", () => ({
  ref: (db: unknown, p: string) => mockRef(db, p),
  onValue: (r: unknown, onData: (snap: unknown) => void, onError?: (err: unknown) => void) =>
    mockOnValue(r, onData, onError),
  set: (r: unknown, v: unknown) => mockSet(r, v),
  update: (r: unknown, v: Record<string, unknown>) => mockUpdate(r, v),
}));

jest.mock("firebase/auth", () => ({
  onAuthStateChanged: (auth: unknown, cb: (u: unknown) => void) => mockOnAuthStateChanged(auth, cb),
}));

jest.mock("@/lib/firebase", () => ({
  db: {},
  auth: {},
}));

jest.mock("@/lib/audit", () => ({
  writeAuditEvent: jest.fn(() => Promise.resolve()),
}));

describe("usePumpData", () => {
  beforeEach(() => {
    mockOnValue.mockClear();
    mockSet.mockClear();
    mockUpdate.mockClear();
    mockUnsub.mockClear();
    mockOnAuthStateChanged.mockImplementation((auth: unknown, cb: (u: unknown) => void) => {
      setTimeout(() => cb({ uid: "test-uid", email: "test@example.com" }), 0);
      return mockUnsub;
    });
  });

  it("subscribes to status and control paths when auth is ready", async () => {
    renderHook(() => usePumpData());
    await waitFor(() => {
      expect(mockOnValue).toHaveBeenCalled();
    });
    expect(mockOnValue).toHaveBeenCalledWith(
      expect.anything(),
      expect.any(Function),
      expect.any(Function)
    );
  });

  it("setMode clears countdown one-shots when selecting COUNTDOWN", async () => {
    let statusCb: ((snap: { exists: () => boolean; val: () => unknown }) => void) | null = null;
    mockOnValue.mockImplementation((ref: unknown, onData: (snap: unknown) => void) => {
      statusCb = onData as typeof statusCb;
      return mockUnsub;
    });

    const { result } = renderHook(() => usePumpData());
    await waitFor(() => {
      expect(result.current.authReady).toBe(true);
    });

    await act(async () => {
      result.current.setMode("COUNTDOWN");
    });

    expect(mockUpdate).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({
        mode: "COUNTDOWN",
        countdown_start: false,
        countdown_add_time: false,
        countdown_stop: false,
        countdown_add_min: 0,
      })
    );
  });

  it("startCountdown clamps duration to 1–120 and writes duration, mode, and countdown_start", async () => {
    mockOnValue.mockReturnValue(mockUnsub);
    const { result } = renderHook(() => usePumpData());
    await waitFor(() => {
      expect(result.current.authReady).toBe(true);
    });

    mockSet.mockClear();
    mockUpdate.mockClear();
    await act(async () => {
      result.current.startCountdown(200);
    });

    expect(mockUpdate).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({
        countdown_duration_min: 120,
        mode: "COUNTDOWN",
        countdown_start: true,
        countdown_stop: false,
      })
    );
  });

  it("startCountdown uses 1 when duration is 0 or negative", async () => {
    mockOnValue.mockReturnValue(mockUnsub);
    const { result } = renderHook(() => usePumpData());
    await waitFor(() => {
      expect(result.current.authReady).toBe(true);
    });

    mockSet.mockClear();
    mockUpdate.mockClear();
    await act(async () => {
      result.current.startCountdown(0);
    });

    const updateCalls = mockUpdate.mock.calls as Array<[unknown, Record<string, unknown>]>;
    expect(updateCalls[0]?.[1]?.countdown_duration_min).toBe(1);
  });

  it("stopCountdown writes countdown_stop one-shot", async () => {
    mockOnValue.mockReturnValue(mockUnsub);
    const { result } = renderHook(() => usePumpData());
    await waitFor(() => {
      expect(result.current.authReady).toBe(true);
    });

    mockSet.mockClear();
    mockUpdate.mockClear();
    await act(async () => {
      result.current.stopCountdown();
    });

    expect(mockUpdate).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({
        countdown_stop: true,
        countdown_start: false,
      })
    );
  });

  it("setBypassFlowSensor writes bypass_flow_sensor field", async () => {
    mockOnValue.mockReturnValue(mockUnsub);
    const { result } = renderHook(() => usePumpData());
    await waitFor(() => {
      expect(result.current.authReady).toBe(true);
    });

    mockSet.mockClear();
    await act(async () => {
      await result.current.setBypassFlowSensor(true);
    });

    const values = mockSet.mock.calls.map((c: unknown[]) => c[1]);
    expect(values).toContain(true);
  });
});
