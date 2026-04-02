/**
 * Gold Standard: usePendingControl hook — mode confirmation and toast on match.
 */
import { renderHook, act } from "@testing-library/react";
import { usePendingControl } from "@/lib/usePendingControl";
import type { PumpControl } from "@/lib/types";

// Mock toast so we can assert it was called
const mockToast = jest.fn();
jest.mock("@/lib/toast", () => ({
  toast: (msg: unknown) => mockToast(msg),
}));

describe("usePendingControl", () => {
  beforeEach(() => {
    mockToast.mockClear();
  });

  it("initializes with null pendingMode and false pendingAck", () => {
    const { result } = renderHook(() => usePendingControl("AUTO"));
    expect(result.current.pendingMode).toBeNull();
    expect(result.current.pendingAck).toBe(false);
  });

  it("setPendingMode updates pendingMode", () => {
    const { result } = renderHook(() => usePendingControl("AUTO"));
    act(() => {
      result.current.setPendingMode("COUNTDOWN");
    });
    expect(result.current.pendingMode).toBe("COUNTDOWN");
  });

  it("when currentMode equals pendingMode, clears pendingMode and shows toast", () => {
    const { result, rerender } = renderHook(
      ({ currentMode }) => usePendingControl(currentMode),
      { initialProps: { currentMode: "AUTO" as PumpControl["mode"] } }
    );
    act(() => {
      result.current.setPendingMode("COUNTDOWN");
    });
    expect(result.current.pendingMode).toBe("COUNTDOWN");

    rerender({ currentMode: "COUNTDOWN" as PumpControl["mode"] });
    expect(result.current.pendingMode).toBeNull();
    expect(mockToast).toHaveBeenCalledWith(
      expect.objectContaining({
        kind: "success",
        title: "Mode confirmed: COUNTDOWN",
      })
    );
  });

  it("does not clear pendingMode when currentMode differs", () => {
    const { result, rerender } = renderHook(
      ({ currentMode }) => usePendingControl(currentMode),
      { initialProps: { currentMode: "AUTO" as PumpControl["mode"] } }
    );
    act(() => {
      result.current.setPendingMode("MANUAL");
    });
    rerender({ currentMode: "AUTO" });
    expect(result.current.pendingMode).toBe("MANUAL");
    expect(mockToast).not.toHaveBeenCalled();
  });
});
