import type { Config } from 'tailwindcss'
import defaultTheme from 'tailwindcss/defaultTheme'

export default <Partial<Config>>{
  theme: {
    extend: {
      fontFamily: {
        sans: ['DM Sans', ...defaultTheme.fontFamily.sans]
      },
      colors: {
        'abc': {
          '50': '#fffbea',
          '100': '#fff5c5',
          '200': '#ffea87',
          '300': '#ffd948',
          '400': '#ffc61e',
          '500': '#f29d03',
          '600': '#df7b00',
          '700': '#b95504',
          '800': '#96410a',
          '900': '#7b360c',
          '950': '#471a01',
        },
        'doge': {
          '50': '#f4f4f2',
          '100': '#e2e4dd',
          '200': '#cbccbc',
          '300': '#adae96',
          '400': '#969778',
          '500': '#88886a',
          '600': '#74725a',
          '700': '#5e5a4a',
          '800': '#524e41',
          '900': '#49453c',
          '950': '#282520',
        },
      }
    }
  }
}
