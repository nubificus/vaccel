export default {
  extends: ['@commitlint/config-conventional'],
  helpUrl: 'https://www.conventionalcommits.org/',
  ignores: [
    (msg) => /Signed-off-by: dependabot\[bot]/m.test(msg),
  ],
  plugins: [
    {
      rules: {
        'body-max-line-length': (parsed, when, value) => {
          const { body } = parsed;
          if (!body) return [true, ''];

          const trailerPattern = /^(Signed-off-by|Co-authored-by|Reviewed-by|Approved-by):/;
          const violation = body
            .split('\n')
            .find((line) => line.length > value && !trailerPattern.test(line));

          return [
            !violation,
            violation
              ? `body's lines must not be longer than ${value} characters (found: "${violation}")`
              : '',
          ];
        },
      },
    },
  ],
  rules: {
    'header-max-length': [2, 'always', 72],
    'body-max-line-length': [2, 'always', 80],
    'subject-case': [2, 'never', [
        "lower-case", // lower case
        "upper-case", // UPPERCASE
        "camel-case", // camelCase
        "kebab-case", // kebab-case
        "pascal-case", // PascalCase
        "snake-case", // snake_case
        "start-case", // Start Case
    ]],
    'trailer-exists': [2, 'always', 'Signed-off-by:'],
  },
}
