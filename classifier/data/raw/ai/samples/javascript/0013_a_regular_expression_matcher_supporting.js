function isMatch(text, pattern) {
  const rows = text.length + 1;
  const cols = pattern.length + 1;
  const dp = Array.from({ length: rows }, () => Array(cols).fill(false));

  dp[0][0] = true;

  for (let j = 2; j < cols; j++) {
    if (pattern[j - 1] === "*") {
      dp[0][j] = dp[0][j - 2];
    }
  }

  for (let i = 1; i < rows; i++) {
    for (let j = 1; j < cols; j++) {
      const textChar = text[i - 1];
      const patternChar = pattern[j - 1];

      if (patternChar === "." || patternChar === textChar) {
        dp[i][j] = dp[i - 1][j - 1];
      } else if (patternChar === "*") {
        dp[i][j] = dp[i][j - 2];

        const preceding = pattern[j - 2];
        if (preceding === "." || preceding === textChar) {
          dp[i][j] ||= dp[i - 1][j];
        }
      }
    }
  }

  return dp[text.length][pattern.length];
}

function main() {
  const examples = [
    ["aa", "a", false],
    ["aa", "a*", true],
    ["ab", ".*", true],
    ["aab", "c*a*b", true],
    ["mississippi", "mis*is*p*.", false],
  ];

  for (const [text, pattern, expected] of examples) {
    const result = isMatch(text, pattern);
    console.log(`isMatch(${JSON.stringify(text)}, ${JSON.stringify(pattern)}) = ${result} (expected ${expected})`);
  }
}

if (require.main === module) {
  main();
}

module.exports = { isMatch };
