#pragma once

class PumpApp {
public:
  /**
   * @brief Executes the core pump control state machine.
   *
   * Evaluates safety gates (emergency stop, dry-run, overflow, sensor freshness),
   * determines the active run mode (AUTO, MANUAL, COUNTDOWN), and actuates
   * the pump relay accordingly. Called once per loop iteration from main.cpp.
   */
  static void executeLogic();

private:
  /**
   * @brief Transitions the system into a safe error-fallback state.
   *
   * Stops the pump, cancels any active countdown, reverts the mode to MANUAL OFF,
   * and synchronizes the fallback state to the cloud. This is a shared recovery
   * path used by all safety-critical error handlers.
   */
  static void enterErrorFallback();
};
