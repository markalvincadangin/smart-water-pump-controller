/**
 * Gold Standard: usePendingControl hook — mode confirmation and toast on match.
 */
import { renderHook, act } from "@testing-library/react";
import { usePendingControl } from "@/lib/usePendingControl";

// Mock toast so we can assert it was called
const toastSpy = jest.fn();
jest.mock("@/lib/toast", () => ({
  toast: (msg: unknown) => toastSpy(msg),
}));

describe("usePendingControl", () => {
  beforeEach(() => {
    toastSpy.mockClear();
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
      { initialProps: { currentMode: "AUTO" as const } }
    );
    act(() => {
      result.current.setPendingMode("COUNTDOWN");
    });
    expect(result.current.pendingMode).toBe("COUNTDOWN");

    rerender({ currentMode: "COUNTDOWN" });
    expect(result.current.pendingMode).toBeNull();
    expect(toastSpy).toHaveBeenCalledWith(
      expect.objectContaining({
        kind: "success",
        title: "Mode confirmed: COUNTDOWN",
      })
    );
  });

  it("does not clear pendingMode when currentMode differs", () => {
    const { result, rerender } = renderHook(
      ({ currentMode }) => usePendingControl(currentMode),
      { initialProps: { currentMode: "AUTO" as const } }
    );
    act(() => {
      result.current.setPendingMode("FORCE_ON");
    });
    rerender({ currentMode: "AUTO" });
    expect(result.current.pendingMode).toBe("FORCE_ON");
    expect(toastSpy).not.toHaveBeenCalled();
  });
});
