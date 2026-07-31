'use strict';

/**
 * Severity levels supported by the logger.
 * @readonly
 * @enum {number}
 */
const Severity = Object.freeze({
  DEBUG: 10,
  INFO: 20,
  WARN: 30,
  ERROR: 40,
  FATAL: 50,
});

/**
 * Ring-buffer based logger with severity filtering.
 */
class RingBufferLogger {
  /**
   * Creates a ring-buffer logger.
   * @param {Object} [options] Logger configuration.
   * @param {number} [options.capacity=1000] Maximum number of retained entries.
   * @param {number} [options.minSeverity=Severity.DEBUG] Minimum severity to record.
   * @param {(entry: Object) => void} [options.onEntry] Callback invoked for each recorded entry.
   */
  constructor({
    capacity = 1000,
    minSeverity = Severity.DEBUG,
    onEntry = null,
  } = {}) {
    if (!Number.isInteger(capacity) || capacity <= 0) {
      throw new RangeError('capacity must be a positive integer');
    }

    this.capacity = capacity;
    this.minSeverity = minSeverity;
    this.onEntry = onEntry;
    this.buffer = new Array(capacity);
    this.nextIndex = 0;
    this.size = 0;
  }

  /**
   * Records a message at the specified severity.
   * @param {number} severity Severity level of the message.
   * @param {string} message Log message.
   * @param {Object} [metadata] Additional structured metadata.
   * @returns {Object|null} The stored entry, or null if filtered out.
   */
  log(severity, message, metadata = {}) {
    if (severity < this.minSeverity) {
      return null;
    }

    const entry = Object.freeze({
      timestamp: new Date().toISOString(),
      severity,
      level: this.levelName(severity),
      message: String(message),
      metadata: { ...metadata },
    });

    this.buffer[this.nextIndex] = entry;
    this.nextIndex = (this.nextIndex + 1) % this.capacity;
    this.size = Math.min(this.size + 1, this.capacity);

    if (typeof this.onEntry === 'function') {
      this.onEntry(entry);
    }

    return entry;
  }

  /**
   * Records a debug-level message.
   * @param {string} message Log message.
   * @param {Object} [metadata] Additional structured metadata.
   * @returns {Object|null} The stored entry, or null if filtered out.
   */
  debug(message, metadata) {
    return this.log(Severity.DEBUG, message, metadata);
  }

  /**
   * Records an info-level message.
   * @param {string} message Log message.
   * @param {Object} [metadata] Additional structured metadata.
   * @returns {Object|null} The stored entry, or null if filtered out.
   */
  info(message, metadata) {
    return this.log(Severity.INFO, message, metadata);
  }

  /**
   * Records a warning-level message.
   * @param {string} message Log message.
   * @param {Object} [metadata] Additional structured metadata.
   * @returns {Object|null} The stored entry, or null if filtered out.
   */
  warn(message, metadata) {
    return this.log(Severity.WARN, message, metadata);
  }

  /**
   * Records an error-level message.
   * @param {string} message Log message.
   * @param {Object} [metadata] Additional structured metadata.
   * @returns {Object|null} The stored entry, or null if filtered out.
   */
  error(message, metadata) {
    return this.log(Severity.ERROR, message, metadata);
  }

  /**
   * Records a fatal-level message.
   * @param {string} message Log message.
   * @param {Object} [metadata] Additional structured metadata.
   * @returns {Object|null} The stored entry, or null if filtered out.
   */
  fatal(message, metadata) {
    return this.log(Severity.FATAL, message, metadata);
  }

  /**
   * Returns retained entries in chronological order.
   * @returns {Object[]} A new array containing retained log entries.
   */
  entries() {
    const start = this.size === this.capacity ? this.nextIndex : 0;
    return Array.from(
      { length: this.size },
      (_, index) => this.buffer[(start + index) % this.capacity],
    );
  }

  /**
   * Removes all retained log entries.
   * @returns {void}
   */
  clear() {
    this.buffer.fill(undefined);
    this.nextIndex = 0;
    this.size = 0;
  }

  /**
   * Returns the number of retained entries.
   * @returns {number} Number of retained entries.
   */
  count() {
    return this.size;
  }

  /**
   * Converts a numeric severity to its name.
   * @param {number} severity Severity value.
   * @returns {string} Severity name, or "UNKNOWN".
   */
  levelName(severity) {
    return Object.keys(Severity).find(
      (name) => Severity[name] === severity,
    ) || 'UNKNOWN';
  }
}

module.exports = {
  RingBufferLogger,
  Severity,
};
