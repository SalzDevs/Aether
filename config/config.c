#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#define INITIAL_SENSOR_CAPACITY 8

// Intermediate representation of one metric entry in the YAML file
typedef struct {
  char name[METRIC_NAME_MAX];
  char unit[METRIC_UNIT_MAX];
  float value;
} MetricSpec;

// Intermediate representation of one sensor entry in the YAML file
typedef struct {
  int id;
  bool hasId;
  char name[SENSOR_NAME_MAX];
  MetricSpec metrics[MAX_METRICS];
  size_t metricCount;
} SensorSpec;

static bool expectEvent(yaml_parser_t* parser, yaml_event_type_t type) {
  yaml_event_t event;
  if (!yaml_parser_parse(parser, &event)) return false;
  bool ok = event.type == type;
  yaml_event_delete(&event);
  return ok;
}

// Reads the current scalar value into out (truncating safely).
static bool readScalarInto(yaml_parser_t* parser, char* out, size_t outSize) {
  yaml_event_t event;
  if (!yaml_parser_parse(parser, &event)) return false;
  bool ok = event.type == YAML_SCALAR_EVENT;
  if (ok) snprintf(out, outSize, "%s", (const char*)event.data.scalar.value);
  yaml_event_delete(&event);
  return ok;
}

static bool readFloat(yaml_parser_t* parser, float* out) {
  char buf[64];
  if (!readScalarInto(parser, buf, sizeof(buf))) return false;
  *out = strtof(buf, NULL);
  return true;
}

static bool readInt(yaml_parser_t* parser, int* out) {
  char buf[64];
  if (!readScalarInto(parser, buf, sizeof(buf))) return false;
  *out = (int)strtol(buf, NULL, 10);
  return true;
}

// Consumes and discards one complete node: either a scalar or a whole
// mapping/sequence block. Used to tolerate unknown keys.
static bool skipNode(yaml_parser_t* parser) {
  yaml_event_t event;
  if (!yaml_parser_parse(parser, &event)) return false;

  if (event.type == YAML_SCALAR_EVENT) {
    yaml_event_delete(&event);
    return true;
  }

  int depth = 0;
  while (depth > 0 ||
         event.type == YAML_MAPPING_START_EVENT ||
         event.type == YAML_SEQUENCE_START_EVENT) {
    if (event.type == YAML_MAPPING_START_EVENT ||
        event.type == YAML_SEQUENCE_START_EVENT) {
      depth++;
    } else if (event.type == YAML_MAPPING_END_EVENT ||
               event.type == YAML_SEQUENCE_END_EVENT) {
      depth--;
    } else if (event.type != YAML_SCALAR_EVENT) {
      yaml_event_delete(&event);
      return false;
    }
    yaml_event_delete(&event);
    if (depth == 0) return true;
    if (!yaml_parser_parse(parser, &event)) return false;
  }

  yaml_event_delete(&event);
  return false;
}

// Parses the body of one metric mapping. The MAPPING_START event must have
// been consumed by the caller.
static bool parseMetricBody(yaml_parser_t* parser, MetricSpec* metric) {
  memset(metric, 0, sizeof(*metric));

  yaml_event_t event;
  while (true) {
    if (!yaml_parser_parse(parser, &event)) return false;

    if (event.type == YAML_MAPPING_END_EVENT) {
      yaml_event_delete(&event);
      return true;
    }
    if (event.type != YAML_SCALAR_EVENT) {
      yaml_event_delete(&event);
      return false;
    }
    char key[32];
    snprintf(key, sizeof(key), "%s", (const char*)event.data.scalar.value);
    yaml_event_delete(&event);

    if (strcmp(key, "name") == 0) {
      if (!readScalarInto(parser, metric->name, sizeof(metric->name))) return false;
    } else if (strcmp(key, "unit") == 0) {
      if (!readScalarInto(parser, metric->unit, sizeof(metric->unit))) return false;
    } else if (strcmp(key, "value") == 0) {
      if (!readFloat(parser, &metric->value)) return false;
    } else {
      if (!skipNode(parser)) return false;
    }
  }
}

// Parses the body of one sensor mapping. The MAPPING_START event must have
// been consumed by the caller.
static bool parseSensorBody(yaml_parser_t* parser, SensorSpec* spec) {
  memset(spec, 0, sizeof(*spec));
  spec->hasId = false;

  yaml_event_t event;
  while (true) {
    if (!yaml_parser_parse(parser, &event)) return false;

    if (event.type == YAML_MAPPING_END_EVENT) {
      yaml_event_delete(&event);
      return true;
    }
    if (event.type != YAML_SCALAR_EVENT) {
      yaml_event_delete(&event);
      return false;
    }
    char key[32];
    snprintf(key, sizeof(key), "%s", (const char*)event.data.scalar.value);
    yaml_event_delete(&event);

    if (strcmp(key, "id") == 0) {
      int id;
      if (!readInt(parser, &id)) return false;
      spec->id = id;
      spec->hasId = true;
    } else if (strcmp(key, "name") == 0) {
      if (!readScalarInto(parser, spec->name, sizeof(spec->name))) return false;
    } else if (strcmp(key, "metrics") == 0) {
      if (!expectEvent(parser, YAML_SEQUENCE_START_EVENT)) return false;

      while (true) {
        if (!yaml_parser_parse(parser, &event)) return false;
        if (event.type == YAML_SEQUENCE_END_EVENT) {
          yaml_event_delete(&event);
          break;
        }
        if (event.type != YAML_MAPPING_START_EVENT) {
          yaml_event_delete(&event);
          return false;
        }
        yaml_event_delete(&event);

        MetricSpec metric;
        if (!parseMetricBody(parser, &metric)) return false;
        if (spec->metricCount < MAX_METRICS) {
          spec->metrics[spec->metricCount++] = metric;
        }
        // metrics beyond MAX_METRICS are parsed and dropped
      }
    } else {
      if (!skipNode(parser)) return false;
    }
  }
}

static sensorData* fileToSensors(const char* fileName, size_t* count) {
  FILE* fptr = fopen(fileName, "r");
  if (fptr == NULL) return NULL;

  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) {
    fclose(fptr);
    return NULL;
  }
  yaml_parser_set_input_file(&parser, fptr);

  size_t capacity = INITIAL_SENSOR_CAPACITY;
  sensorData* sensors = malloc(capacity * sizeof(sensorData));
  *count = 0;
  bool ok = false;

  do {
    if (!expectEvent(&parser, YAML_STREAM_START_EVENT)) break;
    if (!expectEvent(&parser, YAML_DOCUMENT_START_EVENT)) break;
    // root mapping: { sensors: [ ... ] }
    if (!expectEvent(&parser, YAML_MAPPING_START_EVENT)) break;

    // root mapping: tolerate unknown keys, act on "sensors"
    bool foundSensors = false;
    bool rootOk = true;
    while (true) {
      yaml_event_t event;
      if (!yaml_parser_parse(&parser, &event)) { rootOk = false; break; }
      if (event.type == YAML_MAPPING_END_EVENT) {
        yaml_event_delete(&event);
        break;
      }
      if (event.type != YAML_SCALAR_EVENT) {
        yaml_event_delete(&event);
        rootOk = false;
        break;
      }
      char rootKey[32];
      snprintf(rootKey, sizeof(rootKey), "%s",
               (const char*)event.data.scalar.value);
      yaml_event_delete(&event);

      if (strcmp(rootKey, "sensors") == 0 && !foundSensors) {
        if (!expectEvent(&parser, YAML_SEQUENCE_START_EVENT)) { rootOk = false; break; }

        bool sequenceOk = true;
        while (true) {
          yaml_event_t entry;
          if (!yaml_parser_parse(&parser, &entry)) { sequenceOk = false; break; }

          if (entry.type == YAML_SEQUENCE_END_EVENT) {
            yaml_event_delete(&entry);
            break;
          }
          if (entry.type != YAML_MAPPING_START_EVENT) {
            yaml_event_delete(&entry);
            sequenceOk = false;
            break;
          }
          yaml_event_delete(&entry);

          if (*count == capacity) {
            // grow dynamically: the registry has no hard sensor limit
            capacity *= 2;
            sensorData* grown = realloc(sensors, capacity * sizeof(sensorData));
            if (grown == NULL) { sequenceOk = false; break; }
            sensors = grown;
          }

          SensorSpec spec;
          if (!parseSensorBody(&parser, &spec)) {
            sequenceOk = false;
            break;
          }
          if (!spec.hasId) continue; // a sensor without an id is not usable

          sensorData* s = &sensors[*count];
          initSensorData(s);
          s->id = spec.id;
          snprintf(s->name, sizeof(s->name), "%s", spec.name);

          for (size_t i = 0; i < spec.metricCount; i++) {
            addMetric(s, spec.metrics[i].name, spec.metrics[i].unit,
                      spec.metrics[i].value);
          }
          (*count)++;
        }

        if (!sequenceOk) { rootOk = false; break; }
        foundSensors = true;
      } else {
        if (!skipNode(&parser)) { rootOk = false; break; }
      }
    }

    if (!rootOk || !foundSensors) break;
    if (!expectEvent(&parser, YAML_DOCUMENT_END_EVENT)) break;
    if (!expectEvent(&parser, YAML_STREAM_END_EVENT)) break;

    ok = true;
  } while (false);

  yaml_parser_delete(&parser);
  fclose(fptr);

  if (!ok || *count == 0) {
    free(sensors);
    return NULL;
  }

  return sensors;
}

bool LoadSensorConfig(const char *fileName, size_t *count, sensorData **out) {
  sensorData *dt = fileToSensors(fileName, count);
  if (dt == NULL) return false;
  *out = dt;
  return true;
}

bool LoadSettings(const char* fileName, int* outTheme) {
  FILE* fptr = fopen(fileName, "r");
  if (fptr == NULL) return false;

  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) { fclose(fptr); return false; }
  yaml_parser_set_input_file(&parser, fptr);

  bool ok = false, foundTheme = false;
  int theme = 0;

  do {
    if (!expectEvent(&parser, YAML_STREAM_START_EVENT)) break;
    if (!expectEvent(&parser, YAML_DOCUMENT_START_EVENT)) break;
    if (!expectEvent(&parser, YAML_MAPPING_START_EVENT)) break;

    while (true) {
      yaml_event_t event;
      if (!yaml_parser_parse(&parser, &event)) break;
      if (event.type == YAML_MAPPING_END_EVENT) {
        yaml_event_delete(&event);
        break;
      }
      if (event.type != YAML_SCALAR_EVENT) {
        yaml_event_delete(&event);
        break;
      }
      char key[32];
      snprintf(key, sizeof(key), "%s", (const char*)event.data.scalar.value);
      yaml_event_delete(&event);

      if (strcmp(key, "theme") == 0) {
        if (!readInt(&parser, &theme)) break;
        foundTheme = true;
      } else {
        if (!skipNode(&parser)) break;   // tolerate unknown keys
      }
    }

    if (!expectEvent(&parser, YAML_DOCUMENT_END_EVENT)) break;
    if (!expectEvent(&parser, YAML_STREAM_END_EVENT)) break;
    ok = foundTheme;
  } while (false);

  yaml_parser_delete(&parser);
  fclose(fptr);

  if (ok) *outTheme = theme;
  return ok;
}


bool SaveSettings(const char* fileName, int theme) {
  FILE* f = fopen(fileName, "w");
  if (f == NULL) return false;

  yaml_emitter_t emitter;
  if (!yaml_emitter_initialize(&emitter)) { fclose(f); return false; }
  yaml_emitter_set_output_file(&emitter, f);

  bool ok = true;
  yaml_event_t e;
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", theme);

  #define EMIT(ev) do { if (!yaml_emitter_emit(&emitter, &ev)) { ok = false; goto cleanup; } } while (0)

  yaml_stream_start_event_initialize(&e, YAML_UTF8_ENCODING);
  EMIT(e);

  yaml_document_start_event_initialize(&e, NULL, NULL, NULL, 0);
  EMIT(e);

  yaml_mapping_start_event_initialize(&e, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE);
  EMIT(e);

  yaml_scalar_event_initialize(&e, NULL, NULL, (yaml_char_t*)"theme", -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
  EMIT(e);

  yaml_scalar_event_initialize(&e, NULL, NULL, (yaml_char_t*)buf, -1, 1, 0, YAML_PLAIN_SCALAR_STYLE);
  EMIT(e);

  yaml_mapping_end_event_initialize(&e);
  EMIT(e);

  yaml_document_end_event_initialize(&e, 0);
  EMIT(e);

  yaml_stream_end_event_initialize(&e);
  EMIT(e);

  #undef EMIT

cleanup:
  yaml_emitter_flush(&emitter);
  yaml_emitter_delete(&emitter);
  fclose(f);
  return ok;
}
