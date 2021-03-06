#include "modeling_model.h"
#include "modeling_object.h"
#include "serial.h"
#include "platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


Serial file = serial_make((char *)malloc(10000), 10000);

static void _serialize_former(Former *former) {
    serial_write_f32(&file, &former->x, 1); /* former position */
    Curve *curves = former->shape.curves;
    for (int k = 0; k < SHAPE_CURVES; ++k) { /* curves */
        serial_write_f64(&file, &curves[k].x, 1);
        serial_write_f64(&file, &curves[k].y, 1);
        serial_write_f64(&file, &curves[k].w, 1);
    }
}

static void _deserialize_former(Former *former) {
    serial_read_f32(&file, &former->x, 1); /* former position */
    Curve *curves = former->shape.curves;
    for (int k = 0; k < SHAPE_CURVES; ++k) { /* curves */
        serial_read_f64(&file, &curves[k].x, 1);
        serial_read_f64(&file, &curves[k].y, 1);
        serial_read_f64(&file, &curves[k].w, 1);
    }
    shape_update_curve_control_points(curves);
}

void model_serial_dump(Model *model, const char *path) {
    serial_clear(&file);

    /* objects */
    serial_write_i32(&file, &model->objects_count, 1);
    for (int i = 0; i < model->objects_count; ++i) {
        Object *o = model->objects[i];

        /* object position */
        serial_write_f32(&file, o->p.v, 3);

        /* collision formers */
        serial_write_i32(&file, &o->def.formers_count, 1);
        for (int j = 0; j < o->def.formers_count; ++j)
            _serialize_former(o->def.formers + j);

        /* skin formers */
        _serialize_former(&o->def.t_skin_former);
        _serialize_former(&o->def.n_skin_former);
        serial_write_f32(&file, &o->def.t_endp_dx, 1);
        serial_write_f32(&file, &o->def.n_endp_dx, 1);
    }

    serial_write_to_file(&file, path);
}

void model_serial_load(Model *model, const char *path) {
    model_clear(model);

    serial_read_from_file(&file, path);

    /* objects */
    int objects_count;
    serial_read_i32(&file, &objects_count, 1);
    for (int i = 0; i < objects_count; ++i) {
        Object *o = new Object();
        model_add_object(model, o);

        /* object position */
        serial_read_f32(&file, o->p.v, 3);

        /* collision formers */
        serial_read_i32(&file, &o->def.formers_count, 1);
        for (int j = 0; j < o->def.formers_count; ++j)
            _deserialize_former(o->def.formers + j);

        /* skin formers */
        _deserialize_former(&o->def.t_skin_former);
        _deserialize_former(&o->def.n_skin_former);
        serial_read_f32(&file, &o->def.t_endp_dx, 1);
        serial_read_f32(&file, &o->def.n_endp_dx, 1);

        object_finish(o);
    }
}

void model_serial_dump_mesh(Model *model, const char *path) {
    FILE *file = (FILE *)platform_fopen(path, "w");

    /* vertices */
    for (int i = 0; i < model->skin_verts_count; ++i)
        fprintf(file, "%g %g %g\n", model->skin_verts[i].x, model->skin_verts[i].y, model->skin_verts[i].z);

    /* panels */
    for (int i = 0; i < model->panels_count; ++i)
        fprintf(file, "%d %d %d %d\n", model->panels[i].v1, model->panels[i].v2, model->panels[i].v3, model->panels[i].v4);

    fclose(file);
}
