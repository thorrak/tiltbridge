# Adding a New Cloud Target to TiltBridge UI

This document outlines the complete process for adding a new cloud target integration to the TiltBridge user interface. This assumes that the firmware has already been updated to support the new target.

## Prerequisites

Before beginning UI development, you'll need the following information from the firmware implementation:

### Required Firmware Information

1. **API Endpoint Details**

   Targets do **not** get their own endpoints. Every target is saved through
   the one shared endpoint `PUT /api/settings/targets/`, which replies
   `{"status":"ok"}` on success and `{"status":"error"}` with HTTP 400 on
   failure (`idf_json_send_status()` in `src/idf_json_utils.cpp`).

   The firmware decides which target a request is for by **sniffing the keys
   in the payload** — see `processTargetSettings()` in `src/http_server.cpp`,
   which tests each target's distinctive key in turn and dispatches to that
   target's handler. Two consequences for you:

   - Your payload must contain at least one key unique to your target, or the
     request will be dispatched to the wrong handler (or none).
   - The firmware needs a matching branch in `processTargetSettings()` before
     the UI can save anything. Confirm it is there first.

   What you still need from the firmware side:
   - Request body parameter names and types
   - Which key identifies your target to the dispatcher

2. **Configuration Parameters**
   - Parameter names as they appear in the API
   - Data types (string, integer, boolean)
   - Maximum lengths for string fields
   - Default values
   - Required vs. optional parameters

3. **API Response Structure**
   - How settings are returned in `/api/settings/json/`
   - Parameter names in the response (may differ from request)

4. **Validation Requirements**
   - Field validation rules
   - Acceptable value ranges
   - URL format requirements
   - Any special validation logic

### Example Firmware Information (InfluxDB)
```javascript
// API Endpoint: PUT /api/settings/targets/   (shared by every target)
// Dispatch key: influxdbURL -- processTargetSettings() routes on this
// Request Parameters:
{
  "influxdbURL": "string (max 255 chars, required)",
  "influxdbToken": "string (max 127 chars, required)", 
  "influxdbOrg": "string (max 63 chars, required)",
  "influxdbBucket": "string (max 63 chars, required)",
  "influxdbPushEvery": "integer (optional, default 900)"
}

// Success Response (HTTP 200):
{ "status": "ok" }

// Failure Response (HTTP 400):
{ "status": "error" }

// Settings JSON Response (GET /api/settings/json/):
{
  "influxdbURL": "https://...",
  "influxdbToken": "token123", // May be omitted for security
  "influxdbOrg": "myorg",
  "influxdbBucket": "mybucket", 
  "influxdbPushEvery": 900
}
```

## Implementation Steps

### Step 1: Update ConfigStore (`src/stores/ConfigStore.js`)

#### 1.1 Add Reactive Variables
Add ref variables for each configuration parameter in the store:

```javascript
// New Target Settings
const newTargetURL = ref("");
const newTargetApiKey = ref("");
const newTargetPushEvery = ref(900);
```

#### 1.2 Update getConfig() Method
Add code to load the new target settings from the API response:

```javascript
// In getConfig() function, after existing settings:
// New Target Settings
newTargetURL.value = response.newTargetURL || "";
newTargetApiKey.value = response.newTargetApiKey || "";
newTargetPushEvery.value = response.newTargetPushEvery || 900;
```

#### 1.3 Update clearConfig() Method
Add code to reset the new target settings:

```javascript
// In clearConfig() function:
// New Target Settings
newTargetURL.value = "";
newTargetApiKey.value = "";
newTargetPushEvery.value = 900;
```

#### 1.4 Add Update Method
Do **not** write your own `mande()` call. Every target saves through the
shared `updateTargetConfig(payload, onSuccess)` helper already in the store,
which PUTs to `/api/settings/targets/`, checks for `status === "ok"`, and
manages `configUpdateError` for you:

```javascript
async function updateNewTargetConfig(url, apiKey, pushEvery) {
    await updateTargetConfig({
        newTargetURL: url,
        newTargetApiKey: apiKey,
        newTargetPushEvery: pushEvery,
    }, () => {
        newTargetURL.value = url;
        newTargetApiKey.value = apiKey;
        newTargetPushEvery.value = pushEvery;
    });
}
```

The first argument is the request payload — it must include the key the
firmware dispatches on (see Prerequisites). The second is a callback that
runs only on success, to copy the saved values into the store refs.

`updateInfluxDBConfig()` in `ConfigStore.js` is a working example of exactly
this shape.

#### 1.5 Export Variables and Methods
Add the new variables and method to the return statement:

```javascript
return {
    // ... existing exports ...
    newTargetURL,
    newTargetApiKey,
    newTargetPushEvery,
    // ... existing methods ...
    updateNewTargetConfig,
};
```

### Step 2: Create Vue Component (`src/components/config/Targets/NewTarget.vue`)

Create a new Vue component following the established pattern. Use an existing target component as a template (e.g., `InfluxDB.vue` or `Brewfather.vue`).

#### 2.1 Template Structure
```vue
<template>
  <div class="max-w-7xl mx-auto py-6 sm:px-6 lg:px-8">
    <div class="flex-initial md:container">
      <div class="bg-white overflow-hidden sm:rounded-lg sm:shadow">
        
        <!-- Header -->
        <div class="bg-white px-4 py-5 border-b border-gray-200 sm:px-6">
          <h3 class="text-lg leading-6 font-medium text-gray-900">
            {{ $t('cloud_config.newtarget.header') }}
          </h3>
        </div>

        <form @submit.prevent="submitForm">
          <div class="py-5 px-4">
            <div class="space-y-8 divide-y divide-gray-200">
              <div class="pt-8">
                
                <!-- About Section -->
                <div>
                  <h3 class="text-lg leading-6 font-medium text-gray-900">
                    {{ $t('cloud_config.newtarget.about_header') }}
                  </h3>
                  <p class="mt-1 text-sm text-gray-600">
                    {{ $t('cloud_config.newtarget.about_text') }}
                  </p>
                </div>

                <!-- Form Fields -->
                <div class="mt-6 grid grid-cols-1 gap-y-6 gap-x-4 sm:grid-cols-6">
                  <!-- Add form fields here -->
                </div>
              </div>
            </div>

            <!-- Error Message -->
            <FormErrorMsg :form_error_message="form_error_message" v-if="form_error_message.length > 0" />
          </div>

          <!-- Submit Button -->
          <div class="bg-white px-4 py-5 border-t border-gray-200 sm:px-6 sm:flex sm:flex-row-reverse">
            <button type="submit" class="w-full inline-flex justify-center rounded-md border border-transparent shadow-sm px-4 py-2 bg-blue-600 text-base font-medium text-white hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 sm:ml-3 sm:w-auto sm:text-sm">
              {{ $t('sitewide.update') }}
            </button>
          </div>
        </form>
      </div>
    </div>
  </div>

  <UpdateSuccessfulModal update-successful="update-successful" v-model="alertOpen" />
</template>
```

#### 2.2 Script Setup
```vue
<script setup>
import FormErrorMsg from "@/components/generic/FormErrorMsg.vue";
import { useConfigStore } from "@/stores/ConfigStore";
import { useLoading } from 'vue-loading-overlay'
import UpdateSuccessfulModal from "@/components/config/UpdateSuccessfulModal.vue";
import {onMounted, ref} from "vue";
import { i18n } from "@/main.js";

const $loading = useLoading({});
const updateSuccessful = ref(false);
const alertOpen = ref(false);
const configStore = useConfigStore();
let form_error_message = ref("");

// Reactive form variables
const newTargetURL = ref(configStore.newTargetURL);
const newTargetApiKey = ref(configStore.newTargetApiKey);
const newTargetPushEvery = ref(configStore.newTargetPushEvery);

function updateCachedSettings() {
  newTargetURL.value = configStore.newTargetURL;
  newTargetApiKey.value = configStore.newTargetApiKey;
  newTargetPushEvery.value = configStore.newTargetPushEvery;
}

// Component mounting logic
let loader;
onMounted(() => {
  if(configStore.loaded) {
    updateCachedSettings();
  } else {
    loader = $loading.show({});
    configStore.getConfig().then(() => {
      updateCachedSettings();
      loader.hide();
    }).catch(() => {
      loader.hide();
    });
  }
});

// Form submission
async function submitForm() {
  form_error_message.value = "";
  
  // Add validation logic here
  
  loader = $loading.show({});
  configStore.updateNewTargetConfig(
      newTargetURL.value.trim(),
      newTargetApiKey.value.trim(),
      parseInt(newTargetPushEvery.value)
  ).then(() => {
    updateCachedSettings();
    updateSuccessful.value = !configStore.configUpdateError;
    alertOpen.value = true;
  }).finally(() => {
    loader.hide();
  });
}
</script>
```

### Step 3: Add Router Configuration (`src/router/index.js`)

Add a new route in the `CloudConfigView` children array:

```javascript
{
    path: 'newtarget',
    component: () => import('@/components/config/Targets/NewTarget.vue'),
    name: 'NewTargetConfig',
},
```

**Placement:** Insert in logical order among existing targets (alphabetical or by importance).

### Step 4: Update Navigation (`src/App.vue`)

Add the new target to the navigation menu in the `cloud_target` children array:

```javascript
{ name: 'New Target', route_name: 'NewTargetConfig' },
```

**Placement:** Insert in the same logical order as the router configuration.

### Step 5: Add Internationalization (`src/locales/en.json`)

Add a new section under `cloud_config`:

```json
"newtarget": {
  "header": "Configure New Target",
  "about_header": "About New Target Integration", 
  "about_text": "Description of what this target does and how to set it up...",
  "url": "Target URL",
  "url_desc": "Description of the URL field",
  "api_key": "API Key",
  "api_key_desc": "Description of how to obtain the API key",
  "push_frequency": "Push Frequency (seconds)",
  "push_frequency_desc": "How often to send data (in seconds)",
  "error_missing_url": "URL is required",
  "error_missing_api_key": "API Key is required",
  "error_invalid_url": "Please enter a valid URL starting with http:// or https://",
  "error_invalid_push_frequency": "Push frequency must be between 60 and 86400 seconds"
}
```

## Common Form Field Patterns

### Text Input
```vue
<div class="col-span-6 lg:col-span-4">
  <label for="field-id" class="block text-sm font-medium text-gray-700">
    {{ $t("cloud_config.newtarget.field_name") }}
  </label>
  <div class="mt-1">
    <input type="text" name="field-id" v-model="fieldValue" id="field-id" 
           class="block w-full rounded-md border-gray-300 shadow-sm focus:border-indigo-500 focus:ring-indigo-500 sm:text-sm" />
  </div>
  <p class="mt-2 text-sm text-gray-500">{{ $t("cloud_config.newtarget.field_desc") }}</p>
</div>
```

### Password Input  
```vue
<input type="password" name="field-id" v-model="fieldValue" id="field-id" 
       class="block w-full rounded-md border-gray-300 shadow-sm focus:border-indigo-500 focus:ring-indigo-500 sm:text-sm" />
```

### Number Input
```vue
<input type="number" name="field-id" v-model="fieldValue" id="field-id" 
       min="60" max="86400" 
       class="block w-full rounded-md border-gray-300 shadow-sm focus:border-indigo-500 focus:ring-indigo-500 sm:text-sm" />
```

### URL Input
```vue
<input type="url" name="field-id" v-model="fieldValue" id="field-id" 
       placeholder="https://example.com/api" 
       class="block w-full rounded-md border-gray-300 shadow-sm focus:border-indigo-500 focus:ring-indigo-500 sm:text-sm" />
```

## Common Validation Patterns

### Required Field Validation
```javascript
if (!fieldValue.value || fieldValue.value.trim() === "") {
    form_error_message.value = i18n.global.t('cloud_config.newtarget.error_missing_field');
    return;
}
```

### URL Validation
```javascript
try {
    new URL(urlValue.value);
} catch (e) {
    form_error_message.value = i18n.global.t('cloud_config.newtarget.error_invalid_url');
    return;
}
```

### Number Range Validation
```javascript
if (parseInt(numberValue.value) < 60 || parseInt(numberValue.value) > 86400) {
    form_error_message.value = i18n.global.t('cloud_config.newtarget.error_invalid_range');
    return;
}
```

## Testing Checklist

Before considering the implementation complete, verify:

- [ ] ConfigStore properly loads/saves/clears the new settings
- [ ] Component loads existing settings correctly
- [ ] All form fields work and validate properly
- [ ] Error messages display correctly
- [ ] Success modal appears after successful save
- [ ] Navigation menu item works
- [ ] Route loads the component correctly
- [ ] All strings are internationalized (no hardcoded English text)
- [ ] Form follows the established UI patterns
- [ ] Loading states work during API calls
- [ ] Integration with the actual firmware API works

## File Summary

When adding a new target, you will need to modify/create these files:

1. **Modified Files:**
   - `src/stores/ConfigStore.js` - Add store management
   - `src/router/index.js` - Add route
   - `src/App.vue` - Add navigation item  
   - `src/locales/en.json` - Add translations

2. **New Files:**
   - `src/components/config/Targets/NewTarget.vue` - Main component

This process ensures consistency with the existing codebase and provides a seamless user experience for configuring new cloud targets.