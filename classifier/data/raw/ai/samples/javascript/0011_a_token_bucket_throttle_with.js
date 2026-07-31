'use strict';

class TokenBucketError extends Error {
  constructor(message, code = 'TOKEN_BUCKET_ERROR') {
    super(message);
    this.name = 'TokenBucketError';
    this.code = code;
  }
}

class TokenBucketValidationError extends TokenBucketError {
  constructor(message) {
    super(message, 'INVALID_ARGUMENT');
    this.name = 'TokenBucketValidationError';
  }
}

class TokenBucketThrottle {
  constructor({
    capacity,
    refillRate,
    initialTokens = capacity,
    now = () => Date.now(),
    setTimeoutFn = setTimeout,
    clearTimeoutFn = clearTimeout
  } = {}) {
    this._validatePositiveFinite(capacity, 'capacity');
    this._validatePositiveFinite(refillRate, 'refillRate');
    this._validateNonNegativeFinite(initialTokens, 'initialTokens');

    if (initialTokens > capacity) {
      throw new TokenBucketValidationError(
        'initialTokens must not exceed capacity'
      );
    }

    if (typeof now !== 'function') {
      throw new TokenBucketValidationError('now must be a function');
    }

    if (typeof setTimeoutFn !== 'function') {
      throw new TokenBucketValidationError('setTimeoutFn must be a function');
    }

    if (typeof clearTimeoutFn !== 'function') {
      throw new TokenBucketValidationError('clearTimeoutFn must be a function');
    }

    this.capacity = capacity;
    this.refillRate = refillRate;
    this._tokens = initialTokens;
    this._now = now;
    this._setTimeout = setTimeoutFn;
    this._clearTimeout = clearTimeoutFn;
    this._lastRefillAt = this._readTime();
    this._queue = Promise.resolve();
  }

  tryConsume(tokens = 1) {
    this._validateRequest(tokens);
    this._refill();

    if (this._tokens < tokens) {
      return false;
    }

    this._tokens -= tokens;
    return true;
  }

  async consume(tokens = 1) {
    this._validateRequest(tokens);

    const operation = this._queue.then(() => this._consumeWhenAvailable(tokens));
    this._queue = operation.catch(() => {});
    return operation;
  }

  getRemainingTokens() {
    this._refill();
    return this._tokens;
  }

  get retryAfterMs() {
    this._refill();
    return this._tokens >= 1
      ? 0
      : Math.ceil(((1 - this._tokens) / this.refillRate) * 1000);
  }

  reset(tokens = this.capacity) {
    this._validateNonNegativeFinite(tokens, 'tokens');

    if (tokens > this.capacity) {
      throw new TokenBucketValidationError(
        'tokens must not exceed capacity'
      );
    }

    this._tokens = tokens;
    this._lastRefillAt = this._readTime();
  }

  _consumeWhenAvailable(tokens) {
    return new Promise((resolve, reject) => {
      const attempt = () => {
        try {
          this._refill();

          if (this._tokens >= tokens) {
            this._tokens -= tokens;
            resolve();
            return;
          }

          const missingTokens = tokens - this._tokens;
          const delayMs = Math.max(
            0,
            Math.ceil((missingTokens / this.refillRate) * 1000)
          );

          this._setTimeout(attempt, delayMs);
        } catch (error) {
          reject(error);
        }
      };

      attempt();
    });
  }

  _refill() {
    const currentTime = this._readTime();
    const elapsedMs = currentTime - this._lastRefillAt;

    if (elapsedMs < 0) {
      this._lastRefillAt = currentTime;
      return;
    }

    if (elapsedMs === 0) {
      return;
    }

    this._tokens = Math.min(
      this.capacity,
      this._tokens + (elapsedMs / 1000) * this.refillRate
    );
    this._lastRefillAt = currentTime;
  }

  _readTime() {
    const value = this._now();

    if (!Number.isFinite(value) || value < 0) {
      throw new TokenBucketError(
        'now must return a non-negative finite number',
        'INVALID_CLOCK'
      );
    }

    return value;
  }

  _validateRequest(tokens) {
    this._validatePositiveFinite(tokens, 'tokens');

    if (tokens > this.capacity) {
      throw new TokenBucketValidationError(
        'tokens must not exceed capacity'
      );
    }
  }

  _validatePositiveFinite(value, name) {
    if (typeof value !== 'number' || !Number.isFinite(value) || value <= 0) {
      throw new TokenBucketValidationError(
        `${name} must be a finite number greater than zero`
      );
    }
  }

  _validateNonNegativeFinite(value, name) {
    if (
      typeof value !== 'number' ||
      !Number.isFinite(value) ||
      value < 0
    ) {
      throw new TokenBucketValidationError(
        `${name} must be a finite non-negative number`
      );
    }
  }
}

module.exports = {
  TokenBucketThrottle,
  TokenBucketError,
  TokenBucketValidationError
};
