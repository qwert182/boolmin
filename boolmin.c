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
};

struct output_t {
    int gate_idx; // index in 'gatelist'
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

static void do_devlist(enum device_id_t *devlist, int devices_number, int circuit_inputs_number, int circuit_outputs_number) {
    int gateset[GATE_ID_NUMBER];
    memset(gateset, 0, sizeof gateset);
    int gates_number = 0;
    for (int i = 0; i < devices_number; ++i)
        for (int j = 0; j < devices[devlist[i]].gates_number; ++j) {
            ++gateset[devices[devlist[i]].gates[j]];
            ++gates_number;
        }
    enum gate_id_t gatelist[gates_number];

    int inputs_number = circuit_inputs_number;
    int outputs_number = circuit_outputs_number;
    memset(gatelist, 0, (unsigned)gates_number * sizeof *gatelist);
    for (int i = 0, gate_idx = 0; i < GATE_ID_NUMBER; ++i)
        for (int j = 0; j < gateset[i]; ++j) {
            gatelist[gate_idx++] = i;
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
        for (int j = 0; j < gates[gatelist[i]].inputs_number; ++j)
            inputs[input_idx++].gate_idx = i;
        for (int j = 0; j < gates[gatelist[i]].outputs_number; ++j)
            outputs[output_idx++].gate_idx = i;
    }

    for (int i = 0; i < gates_number; ++i) {
        printf("gate[%d] = %s: ", i, gates[gatelist[i]].name);
        printf("i={");
        int first = 1;
        for (int j = 0; j < inputs_number; ++j)
            if (inputs[j].gate_idx == i)
                printf(first ? "%d" : ", %d", j), first = 0;
        printf("}, ");
        printf("o={");
        first = 1;
        for (int j = 0; j < outputs_number; ++j)
            if (outputs[j].gate_idx == i)
                printf(first ? "%d" : ", %d", j), first = 0;
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
            if (gates[gatelist[i]].is_symmetric)
                total_graphs_number_computed *= multiset_number(outputs_number, gates[gatelist[i]].inputs_number);
            else
                total_graphs_number_computed *= power(outputs_number, gates[gatelist[i]].inputs_number);

    long total_graphs_number = 0;
    for (;;) {
        printf("graph[%ld/%ld] = {", total_graphs_number, total_graphs_number_computed);
        for (int i = 0; i < inputs_number; ++i)
            printf(i ? " %d" : "%d", inputs[i].connection);
        printf("}\n");
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
                        // connect to output[0]
                        inputs[i].connection = 0;
                    } else {
                        if (inputs[i].gate_idx == inputs[i + 1].gate_idx && inputs[i + 1].connection + 1 != outputs_number && gates[gatelist[inputs[i].gate_idx]].is_symmetric) {
                            inputs[i].connection = inputs[i + 1].connection + 1;
                        } else {
                            inputs[i].connection = 0;
                            //if (i-1 >= circuit_inputs_number && inputs[i-1].connection == outputs_number) {
                            //    for (int j = i-1; j >= 0 && inputs[j].gate_idx == inputs[i].gate_idx; --j)
                            //        inputs[j].connection = 0;
                            //}
                        }
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
    //memset(devlist, 0, (unsigned)(max_devices_number + 1) * sizeof *devlist);
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
            do_devlist(devlist, max_devices_number, 1, 1);
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
