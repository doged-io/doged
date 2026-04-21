// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import type { Config } from 'tailwindcss';
import type { PluginAPI } from 'tailwindcss/types/config';

const config: Config = {
    content: [
        './src/pages/**/*.{js,ts,jsx,tsx,mdx}',
        './src/components/**/*.{js,ts,jsx,tsx,mdx}',
        './src/app/**/*.{js,ts,jsx,tsx,mdx}',
    ],
    theme: {
        extend: {
            colors: {
                /** e.cash marketing site palette (web/e.cash) */
                background: '#090916',
                accentLight: '#01a0e0',
                accentMedium: '#0671c0',
                accentDark: '#224da8',
                primaryText: '#f4f4f8',
                secondaryText: '#e2e4eb',
                borderLight: 'rgba(255, 255, 255, 0.1)',
            },
            fontFamily: {
                sans: ['var(--font-space-grotesk)', 'system-ui', 'sans-serif'],
                mono: ['var(--font-fira-code)', 'ui-monospace', 'monospace'],
            },
            typography: ({ theme }: PluginAPI) => ({
                DEFAULT: {
                    css: {
                        '--tw-prose-body': theme('colors.secondaryText'),
                        '--tw-prose-headings': theme('colors.primaryText'),
                        '--tw-prose-bold': theme('colors.primaryText'),
                        '--tw-prose-bullets': theme('colors.accentMedium'),
                        '--tw-prose-counters': theme('colors.accentMedium'),
                        '--tw-prose-captions': theme('colors.secondaryText'),
                        '--tw-prose-code': theme('colors.accentLight'),
                        '--tw-prose-pre-code': theme('colors.secondaryText'),
                        /* Default prose uses slate-900 for quotes — illegible on dark bg */
                        '--tw-prose-quotes': theme('colors.secondaryText'),
                        '--tw-prose-quote-borders': theme(
                            'colors.accentMedium',
                        ),
                        '--tw-prose-hr': 'var(--borderLight)',
                        '--tw-prose-links': theme('colors.accentMedium'),
                        '--tw-prose-th-borders': 'var(--borderLight)',
                        '--tw-prose-td-borders': 'var(--borderLight)',
                        'maxWidth': 'none',
                        'color': theme('colors.secondaryText'),
                        'fontWeight': '400',
                        'lineHeight': '1.65',
                        'a': {
                            'color': theme('colors.accentMedium'),
                            'textDecoration': 'underline',
                            'transitionProperty':
                                'color, text-decoration-color',
                            'transitionDuration': '200ms',
                            '&:hover': {
                                color: theme('colors.accentLight'),
                            },
                        },
                        'h1': {
                            color: theme('colors.primaryText'),
                            fontWeight: '700',
                        },
                        'h2': {
                            color: theme('colors.primaryText'),
                            fontWeight: '700',
                        },
                        'h2 code': {
                            fontFamily: theme('fontFamily.mono'),
                            fontWeight: '600',
                            fontSize: 'inherit',
                            color: theme('colors.accentLight'),
                            backgroundColor: 'rgba(255, 255, 255, 0.06)',
                            paddingLeft: theme('spacing.2'),
                            paddingRight: theme('spacing.2'),
                            paddingTop: theme('spacing.1'),
                            paddingBottom: theme('spacing.1'),
                            borderRadius: theme('borderRadius.md'),
                        },
                        'h3': {
                            color: theme('colors.primaryText'),
                            fontWeight: '700',
                        },
                        'h4': {
                            color: theme('colors.primaryText'),
                        },
                        'strong': {
                            color: theme('colors.primaryText'),
                            fontWeight: '600',
                        },
                        'code': {
                            fontFamily: theme('fontFamily.mono'),
                        },
                        /* @tailwindcss/typography adds literal ` via ::before/::after on code */
                        'code::before': {
                            content: 'none',
                        },
                        'code::after': {
                            content: 'none',
                        },
                        'pre code': {
                            fontFamily: theme('fontFamily.mono'),
                        },
                        'blockquote': {
                            color: theme('colors.secondaryText'),
                            fontStyle: 'normal',
                            backgroundColor: 'rgba(255, 255, 255, 0.04)',
                            borderRadius: theme('borderRadius.md'),
                            paddingTop: theme('spacing.3'),
                            paddingBottom: theme('spacing.3'),
                            paddingRight: theme('spacing.4'),
                        },
                        'blockquote strong': {
                            color: theme('colors.primaryText'),
                            fontWeight: '600',
                        },
                        'blockquote code': {
                            color: theme('colors.accentLight'),
                            fontWeight: '500',
                        },
                        'blockquote a': {
                            'color': theme('colors.accentMedium'),
                            '&:hover': {
                                color: theme('colors.accentLight'),
                            },
                        },
                        'blockquote p': {
                            color: theme('colors.secondaryText'),
                        },
                        'blockquote ul': {
                            color: theme('colors.secondaryText'),
                        },
                    },
                },
            }),
        },
    },
    plugins: [require('@tailwindcss/typography')],
};

export default config;
