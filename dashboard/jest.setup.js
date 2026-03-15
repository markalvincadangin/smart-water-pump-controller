// Mock window for toast and other browser APIs
if (typeof window !== "undefined") {
  window.dispatchEvent = window.dispatchEvent || (() => {});
}

// Firebase is mocked per-test via jest.mock()
