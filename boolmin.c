#include <stddef.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include "boolmin.h"

static struct options_t {
    unsigned enable_symmetric_relaxation : 1;
} options = {
    1, // enable_symmetric_relaxation
};

struct input_t {
    int gate_idx; // index in 'gatelist'
    int connection; // index in 'outputs'
    int visited_by_route; // index in 'inputs' (routes start from inputs[0..circuit_inputs_number-1])
};

struct output_t {
    int gate_idx; // index in 'gatelist'
};

struct gatelist_t {
    enum gate_id_t id;
    int io_indices[5];
};

static void reset_circuit_inputs(struct input_t *inputs, int circuit_inputs_number, int circuit_outputs_number) {
    for (int i = circuit_inputs_number; i > 0; --i) {
        assert(inputs[i - 1].gate_idx == -1);
        // connect to output[<... third, second, first gate>]
        if (i == circuit_inputs_number)
            inputs[i - 1].connection = circuit_outputs_number;
        else
            inputs[i - 1].connection = inputs[i].connection + 1;
    }
}

struct queue_t {
    int head, tail;
    int nodes[30];
};

static void queue_reset(struct queue_t *queue) {
    queue->head = 0;
    queue->tail = 0;
}

static int queue_get_size(struct queue_t *queue) {
    if (queue->head <= queue->tail)
        return queue->tail - queue->head;
    else
        return LENGTH(queue->nodes) - queue->head + queue->tail;
}

static void queue_push(struct queue_t *queue, int value) {
    assert(queue_get_size(queue) < LENGTH(queue->nodes) - 1);
    queue->nodes[queue->tail++] = value;
    if (unlikely(queue->tail == LENGTH(queue->nodes)))
        queue->tail = 0;
}

static int queue_pop(struct queue_t *queue, int *value_out) {
    if (queue_get_size(queue) > 0) {
        *value_out = queue->nodes[queue->head++];
        if (unlikely(queue->head == LENGTH(queue->nodes)))
            queue->head = 0;
        return 1;
    }
    return 0;
}

static void do_devlist(enum device_id_t *devlist, int devices_number, int circuit_inputs_number, int circuit_outputs_number) {
    int gateset[GATE_ID_NUMBER];
    memset(gateset, 0, sizeof gateset);
    int gates_number = 0;
    for (int i = 0; i < devices_number; ++i)
        for (int j = 0; j < devices[devlist[i]].gates_number; ++j) {
            ++gateset[devices[devlist[i]].gates[j]];
            ++gates_number;
        }
    struct gatelist_t gatelist[gates_number];
    memset(gatelist, 0, (unsigned)gates_number * sizeof *gatelist);

    int inputs_number = circuit_inputs_number;
    int outputs_number = circuit_outputs_number;
    for (int i = 0, gate_idx = 0; i < GATE_ID_NUMBER; ++i)
        for (int j = 0; j < gateset[i]; ++j) {
            gatelist[gate_idx++].id = i;
            inputs_number += gates[i].inputs_number;
            outputs_number += gates[i].outputs_number;
        }

    // check that circuit_inputs_number <= (gates outputs number w/o circuit_outputs)
    // remove some circuit_inputs and retry
    assert(circuit_inputs_number <= outputs_number - circuit_outputs_number);

    struct input_t inputs[inputs_number + 1];
    struct output_t outputs[outputs_number];
    memset(inputs, -1, (unsigned)(inputs_number + 1) * sizeof *inputs);
    memset(outputs, -1, (unsigned)outputs_number * sizeof *outputs);
    for (int i = 0, input_idx = circuit_inputs_number, output_idx = circuit_outputs_number; i < gates_number; ++i) {
        assert(LENGTH(gatelist[i].io_indices) >= gates[gatelist[i].id].inputs_number + gates[gatelist[i].id].outputs_number);
        int io_idx = 0;
        for (int j = 0; j < gates[gatelist[i].id].inputs_number; ++j) {
            gatelist[i].io_indices[io_idx++] = input_idx;
            inputs[input_idx++].gate_idx = i;
        }
        for (int j = 0; j < gates[gatelist[i].id].outputs_number; ++j) {
            gatelist[i].io_indices[io_idx++] = output_idx;
            outputs[output_idx++].gate_idx = i;
        }
    }

    for (int i = 0; i < gates_number; ++i) {
        printf("gate[%d] = %s: ", i, gates[gatelist[i].id].name);
        printf("i={");
        for (int j = 0; j < gates[gatelist[i].id].inputs_number; ++j)
            printf(j ? ", %d" : "%d", gatelist[i].io_indices[j]);
        printf("}, ");
        printf("o={");
        for (int j = 0; j < gates[gatelist[i].id].outputs_number; ++j)
            printf(j ? ", %d" : "%d", gatelist[i].io_indices[gates[gatelist[i].id].inputs_number + j]);
        printf("}");
        printf("\n");
    }
    for (int i = 0; i < outputs_number; ++i)
        printf("output[%d] = {%d}\n", i, outputs[i].gate_idx);
    for (int i = 0; i < inputs_number; ++i)
        printf("input[%d] = {%d, %d}\n", i, inputs[i].gate_idx, inputs[i].connection);
    printf("\n");

    reset_circuit_inputs(inputs, circuit_inputs_number, circuit_outputs_number);
    for (int i = circuit_inputs_number; i < inputs_number; ++i) {
        assert(inputs[i].gate_idx >= 0);
        // connect to output[0]
        inputs[i].connection = 0;
    }

    long combinations_of_circuit_inputs = combinations_number(outputs_number - circuit_outputs_number, circuit_inputs_number);
    printf("combinations_of_circuit_inputs = %ld\n", combinations_of_circuit_inputs);
    //long total_graphs_number_computed = //power(outputs_number, inputs_number); // full
    //                                    //  power(outputs_number, inputs_number - circuit_inputs_number) // w/o <circuit inputs> to <circuit_outputs>
    //                                    //* power(outputs_number - circuit_outputs_number, circuit_inputs_number);
    //                                    power(outputs_number, inputs_number - circuit_inputs_number)
    //                                  * combinations_of_circuit_inputs;
    long total_graphs_number_computed = combinations_of_circuit_inputs;
    if (!options.enable_symmetric_relaxation)
        total_graphs_number_computed *= power(outputs_number, inputs_number - circuit_inputs_number);
    else
        for (int i = 0; i < gates_number; ++i)
            if (gates[gatelist[i].id].is_symmetric)
                total_graphs_number_computed *= multiset_number(outputs_number, gates[gatelist[i].id].inputs_number);
            else
                total_graphs_number_computed *= power(outputs_number, gates[gatelist[i].id].inputs_number);

    long total_graphs_number = 0;
    for (;;) {
        printf("graph[%ld/%ld] = {", total_graphs_number, total_graphs_number_computed);
        for (int i = 0; i < inputs_number; ++i)
            printf(i ? " %d" : "%d", inputs[i].connection);
        printf("}\n");

        {
            FILE *f = fopen("/tmp/graph.dot", "w");
            fprintf(f, "digraph G {\n");
            for (int i = 0; i < circuit_inputs_number; ++i)
                fprintf(f, "  i%d [shape=Msquare];\n", i);
            for (int i = 0; i < circuit_outputs_number; ++i)
                fprintf(f, "  o%d [shape=doublecircle];\n", i);
            for (int i = 0; i < gates_number; ++i) {
                fprintf(f, "  subgraph cluster_0g%d {\n", i);
                fprintf(f, "    label = \"g%d\";\n", i);
                fprintf(f, "    labeljust = l;\n");
                fprintf(f, "    style = filled;\n");
                fprintf(f, "    color = lightgrey;\n");
                fprintf(f, "    node [style=filled, fillcolor=white];\n");
                fprintf(f, "    edge [style=invis];\n");
                //for (int j = 0; j < inputs_number; ++j)
                //    if (inputs[j].gate_idx == i)
                //        fprintf(f, "    i%d;\n", j);
                //for (int j = 0; j < outputs_number; ++j)
                //    if (outputs[j].gate_idx == i) {
                //        fprintf(f, "    o%d;\n", j);
                //        for (int k = 0; k < inputs_number; ++k)
                //            if (inputs[k].gate_idx == i)
                //                fprintf(f, "    i%d -> o%d;\n", k, j);
                //    }
                for (int j = 0; j < gates[gatelist[i].id].inputs_number; ++j)
                    fprintf(f, "    i%d;\n", gatelist[i].io_indices[j]);
                for (int j = 0; j < gates[gatelist[i].id].outputs_number; ++j)
                    for (int k = 0; k < gates[gatelist[i].id].inputs_number; ++k)
                        fprintf(f, "    i%d -> o%d;\n", gatelist[i].io_indices[k], gatelist[i].io_indices[gates[gatelist[i].id].inputs_number + j]);
                fprintf(f, "  }\n");
            }
            for (int i = circuit_outputs_number; i < outputs_number; ++i)
                fprintf(f, "  o%d;\n", i);
            for (int i = 0; i < inputs_number; ++i)
                fprintf(f, "  o%d -> i%d;\n", inputs[i].connection, i);
            fprintf(f, "}\n");
            fclose(f);
        }

        {
            for (int i = 0; i < inputs_number; ++i)
                inputs[i].visited_by_route = -1;
            struct queue_t queue;
            queue_reset(&queue);
            for (int input_idx = 0; input_idx < circuit_inputs_number; ++input_idx) {
                int node = input_idx;
                int reached_circuit_output_gate = 0;
                do {
                    inputs[node].visited_by_route = input_idx;
                    int gate_idx = outputs[inputs[node].connection].gate_idx;
                    //if (outputs[inputs[node].connection].visited_by_route == input_idx) {
                    //    // POSSIBLE loop detected
                    //    ------------assert(0);
                    //}
                    printf("node %d --> output %d (of gate_idx = %d)\n", node, inputs[node].connection, gate_idx);
                    if (gate_idx >= 0) {
                        for (int i = 0; i < gates[gatelist[gate_idx].id].inputs_number; ++i) {
                            int to_node = gatelist[gate_idx].io_indices[i];
                            if (inputs[to_node].visited_by_route == -1) {
                                queue_push(&queue, to_node);
                            } else if (inputs[to_node].visited_by_route != input_idx) {
                                printf("skipped node %d (from %d), already visited by route %d, current route %d\n", to_node, node, inputs[to_node].visited_by_route, input_idx);
                                reached_circuit_output_gate = 1;
                            } else {
                                printf("loop detected to node %d (from %d), already visited in route %d\n", to_node, node, input_idx);
                                fflush(stdout);
                                // loop detected
                                //assert(0);
                                goto _big_break;
                            }
                        }
                    } else
                        reached_circuit_output_gate = 1;
                } while (queue_pop(&queue, &node));
                assert(reached_circuit_output_gate);
            }
            _big_break:;
        }

        ++total_graphs_number;
        //if (total_graphs_number == 200000) break;

        int i = 0;
        for (int j = 0; i < circuit_inputs_number; ++i, ++j) {
            assert(inputs[i].gate_idx == -1);
            ++inputs[i].connection;
            if (inputs[i].connection != outputs_number) {
                if (j > 0) {
                    if (inputs[j - 1].connection == inputs[j].connection + 1)
                        continue;
                    do {
                        inputs[j - 1].connection = inputs[j].connection + 1;
                        assert(inputs[j - 1].connection < outputs_number);
                    } while (--j > 0);
                }
                break;
            }
        }
        if (i == circuit_inputs_number) {
            reset_circuit_inputs(inputs, circuit_inputs_number, circuit_outputs_number);
            for (; i < inputs_number; ++i) {
                assert(inputs[i].gate_idx >= 0);
                ++inputs[i].connection;
                if (inputs[i].connection == outputs_number) {
                    if (!options.enable_symmetric_relaxation) {
                        inputs[i].connection = 0;
                    } else {
                        if (inputs[i].gate_idx == inputs[i + 1].gate_idx && inputs[i + 1].connection + 1 != outputs_number && gates[gatelist[inputs[i].gate_idx].id].is_symmetric)
                            inputs[i].connection = inputs[i + 1].connection + 1;
                        else
                            inputs[i].connection = 0;
                    }
                } else
                    break;
            }
        }
        if (i == inputs_number)
            break;
    }

    assert(total_graphs_number == total_graphs_number_computed);
    printf("done %ld graphs\n", total_graphs_number);
}

int main() {
    int max_devices_number = 1;
    enum device_id_t devlist[max_devices_number + 1];
    for (int i = 0; i < max_devices_number; ++i)
        devlist[i] = DEVICE_NONE + 1;
    devlist[max_devices_number] = DEVICE_NONE;

    long total_devsets_number_computed = multiset_number(DEVICE_ID_NUMBER - 1, max_devices_number);
    long total_devsets_number = 0;
    for (;;) {
        printf("devset[%ld/%ld] = {", total_devsets_number, total_devsets_number_computed);
        int first = 1;
        for (enum device_id_t device_id = 0; device_id < DEVICE_ID_NUMBER; ++device_id) {
            int devices_number = 0;
            for (int i = 0; i < max_devices_number; ++i)
                devices_number += devlist[i] == device_id;
            if (devices_number) {
                printf(first ? "%d * %s" : ", %d * %s", devices_number, devices[device_id].name);
                first = 0;
            }
        }
        printf("}\n");
        if (total_devsets_number == 1) {
            do_devlist(devlist, max_devices_number, 2, 1);
            return 0;
        }
        ++total_devsets_number;
        int i = 0;
        for (;; ++i) {
            ++devlist[i];
            if (devlist[i] == DEVICE_ID_NUMBER)
                devlist[i] = devlist[i + 1] + 1;
            else
                break;
        }
        if (i == max_devices_number)
            break;
        for (int j = i - 1; j > 0; --j)
            devlist[j - 1] = devlist[i - 1];
    }
    printf("total_devsets_number = %ld\n", total_devsets_number);
    assert(total_devsets_number == total_devsets_number_computed);
}
