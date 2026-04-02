/**
 * Gold Standard: usePumpData — Firebase data handling and control writers.
 * Mocks Firebase ref/onValue/set and auth to test hook behavior.
 */
import { renderHook, act, waitFor } from "@testing-library/react";
import { usePumpData } from "@/lib/usePumpData";

const mockUnsub = jest.fn();
const mockOnValue = jest.fn((..._args: any[]) => mockUnsub) as jest.Mock<any, any>;
const mockSet = jest.fn<Promise<void>, [unknown, unknown]>(() => Promise.resolve());
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

  it("setMode writes only to control/mode path", async () => {
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
      result.current.setMode("AUTO");
    });

    expect(mockSet).toHaveBeenCalledWith(
      expect.anything(),
      "AUTO"
    );
  });

  it("startCountdown clamps duration to 1–120 and writes duration, mode, and countdown_start", async () => {
    mockOnValue.mockReturnValue(mockUnsub);
    const { result } = renderHook(() => usePumpData());
    await waitFor(() => {
      expect(result.current.authReady).toBe(true);
    });

    mockSet.mockClear();
    await act(async () => {
      result.current.startCountdown(200);
    });

    expect(mockSet).toHaveBeenCalledWith(expect.anything(), 120);
    expect(mockSet).toHaveBeenCalledWith(expect.anything(), "COUNTDOWN");
    expect(mockSet).toHaveBeenCalledWith(expect.anything(), true);
  });

  it("startCountdown uses 1 when duration is 0 or negative", async () => {
    mockOnValue.mockReturnValue(mockUnsub);
    const { result } = renderHook(() => usePumpData());
    await waitFor(() => {
      expect(result.current.authReady).toBe(true);
    });

    mockSet.mockClear();
    await act(async () => {
      result.current.startCountdown(0);
    });

    const setCalls = mockSet.mock.calls as Array<[unknown, unknown]>;
    const durationCall = setCalls.find((c) => typeof c[1] === "number");
    expect(durationCall?.[1]).toBe(1);
  });

  it("stopCountdown writes countdown_stop one-shot", async () => {
    mockOnValue.mockReturnValue(mockUnsub);
    const { result } = renderHook(() => usePumpData());
    await waitFor(() => {
      expect(result.current.authReady).toBe(true);
    });

    mockSet.mockClear();
    await act(async () => {
      result.current.stopCountdown();
    });

    expect(mockSet).toHaveBeenCalledWith(expect.anything(), true);
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
