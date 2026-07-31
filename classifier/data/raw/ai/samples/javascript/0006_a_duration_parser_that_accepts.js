function parseDuration(input) {
  if (typeof input !== "string" || input.length === 0) return NaN;

  let total = 0;
  let number = 0;
  let hasNumber = false;
  let hasUnit = false;

  for (let i = 0; i < input.length; i++) {
    const code = input.charCodeAt(i);

    if (code >= 48 && code <= 57) {
      number = number * 10 + code - 48;
      hasNumber = true;
      continue;
    }

    let multiplier;
    if (code === 100) multiplier = 86400000;
    else if (code === 104) multiplier = 3600000;
    else if (code === 109) multiplier = 60000;
    else if (code === 115) multiplier = 1000;
    else if (code === 77) multiplier = 1;
    else return NaN;

    if (!hasNumber) return NaN;
    total += number * multiplier;
    number = 0;
    hasNumber = false;
    hasUnit = true;
  }

  return hasNumber || !hasUnit ? NaN : total;
}
