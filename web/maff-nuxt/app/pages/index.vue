<script setup lang="ts">
const { data: page } = await useAsyncData('index', () => queryContent('/').findOne())

useSeoMeta({
  title: page.value.title,
  ogTitle: page.value.title,
  description: page.value.description,
  ogDescription: page.value.description
})

</script>

<template>
  <div>
    <ULandingHero
      :title="page.hero.title"
      :links="page.hero.links"
      orientation="horizontal"
    >
      <template #headline>
        <p class="font-bold text-2xl">
          {{ page.hero.headline.label }}
        </p>
      </template>
      <template #description>
        <p class="font-bold">
          {{ page.hero.description }}
        </p>
      </template>
      <template #default>
        <UContainer>
          <UColorModeImage
            :light="page.hero.image.light"
            :dark="page.hero.image.dark"
            as="img"
          />
        </UContainer>
      </template>
    </ULandingHero>

    <ULandingSection
      v-for="section in page.sections"
      v-bind="section"
    >
      <template #description>
        <p class="font-medium py-2 max-w-xl text-left"
          v-for="paragraph in section.description"
        >
          {{ paragraph }}
        </p>
      </template>
      <!-- YouTube embed -->
      <ScriptYouTubePlayer
        v-if="section.video"
        :video-id="section.video.id"
        :title="section.video.title"
      >
      </ScriptYouTubePlayer>
      <!-- light/dark image -->
      <UContainer v-if="section.image">
        <UColorModeImage
          :light="section.image.light"
          :dark="section.image.dark"
          as="img"
        />
      </UContainer>
      <template #bottom>
        <ULandingSection
          v-for="subsection in section.subsections"
          v-bind="subsection"
        />
      </template>
    </ULandingSection>

    <ULandingSection
      :title="page.features.title"
      :description="page.features.description"
      :headline="page.features.headline"
      id="features"
    >
      <template #bottom>
        <UContainer>
          <UPageHeader
            v-bind="page.features.grid.header"
            :ui="{ wrapper: 'border-none' }"
          />
          <UPageGrid
            class="scroll-mt-[calc(var(--header-height)+140px+128px+96px)]"
          >
            <ULandingCard 
              v-for="item in page.features.grid.items"
              v-bind="item"
            >

            </ULandingCard>
          </UPageGrid>
        </UContainer>

      </template>
    </ULandingSection>

    <ULandingSection
      class="bg-primary-50 dark:bg-primary-400 dark:bg-opacity-10"
      :headline="page.cta.headline"
      :title="page.cta.title"
      :card="false"
      align="left"
    >
      <template #description>
        <p class="font-medium py-2 max-w-xl text-left"
          v-for="paragraph in page.cta.description"
        >
          {{ paragraph }}
        </p>
      </template>
      <NuxtImg
        :src="page.cta.image"
        fit="inside"
        style="
          border-radius: 50%; border: 4px solid #cbccbc;
          width: 100%;
        "
      />
    </ULandingSection>

    <ULandingSection
      id="faq"
      :title="page.faq.title"
      :description="page.faq.description"
      class="scroll-mt-[var(--header-height)]"
    >
      <ULandingFAQ
        multiple
        :items="page.faq.items"
        :ui="{
          button: {
            label: 'font-semibold',
            trailingIcon: {
              base: 'w-6 h-6'
            }
          }
        }"
        class="max-w-4xl mx-auto"
      />
    </ULandingSection>
  </div>
</template>
