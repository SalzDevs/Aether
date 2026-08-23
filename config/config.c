#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#define MAX_SENSORS 32

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

  sensorData* sensors = malloc(MAX_SENSORS * sizeof(sensorData));
  *count = 0;
  bool ok = false;

  do {
    if (!expectEvent(&parser, YAML_STREAM_START_EVENT)) break;
    if (!expectEvent(&parser, YAML_DOCUMENT_START_EVENT)) break;
    // root mapping: { sensors: [ ... ] }
    if (!expectEvent(&parser, YAML_MAPPING_START_EVENT)) break;

    char rootKey[32];
    if (!readScalarInto(&parser, rootKey, sizeof(rootKey))) break;
    if (strcmp(rootKey, "sensors") != 0) break;
    if (!expectEvent(&parser, YAML_SEQUENCE_START_EVENT)) break;

    bool sequenceOk = true;
    while (true) {
      yaml_event_t event;
      if (!yaml_parser_parse(&parser, &event)) { sequenceOk = false; break; }

      if (event.type == YAML_SEQUENCE_END_EVENT) {
        yaml_event_delete(&event);
        break;
      }
      if (event.type != YAML_MAPPING_START_EVENT) {
        yaml_event_delete(&event);
        sequenceOk = false;
        break;
      }
      yaml_event_delete(&event);

      SensorSpec spec;
      if (*count >= MAX_SENSORS || !parseSensorBody(&parser, &spec)) {
        // extra entries are consumed but dropped when full
        sequenceOk = (*count < MAX_SENSORS);
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

    if (!sequenceOk) break;
    if (!expectEvent(&parser, YAML_MAPPING_END_EVENT)) break;
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
