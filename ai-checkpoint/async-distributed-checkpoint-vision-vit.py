import os
import time

import torch
import torch.distributed as dist
import torch.distributed.checkpoint as dcp
import torch.multiprocessing as mp
import torch.nn as nn
import torchvision.models as models

from torch.distributed.fsdp import fully_shard
from torch.distributed.checkpoint.state_dict import get_state_dict, set_state_dict
from torch.distributed.checkpoint.stateful import Stateful

CHECKPOINT_DIR = "checkpoint/vis-async"


class AppState(Stateful):
    """This is a useful wrapper for checkpointing the Application State. Since this object is compliant
    with the Stateful protocol, DCP will automatically call state_dict/load_stat_dict as needed in the
    dcp.save/load APIs.

    Note: We take advantage of this wrapper to hande calling distributed state dict methods on the model
    and optimizer.
    """

    def __init__(self, model, optimizer=None):
        self.model = model
        self.optimizer = optimizer

    def state_dict(self):
        # this line automatically manages FSDP FQN's, as well as sets the default state dict type to FSDP.SHARDED_STATE_DICT
        model_state_dict, optimizer_state_dict = get_state_dict(self.model, self.optimizer)
        return {
            "model": model_state_dict,
            "optim": optimizer_state_dict
        }

    def load_state_dict(self, state_dict):
        # sets our state dicts on the model and optimizer, now that we've loaded
        set_state_dict(
            self.model,
            self.optimizer,
            model_state_dict=state_dict["model"],
            optim_state_dict=state_dict["optim"]
        )


def setup(rank, world_size):
    # initialize the process group
    dist.init_process_group("cpu:gloo,cuda:nccl", rank=rank, world_size=world_size)
    torch.cuda.set_device(rank)


def cleanup():
    dist.destroy_process_group()


def run_fsdp_checkpoint_save_example(rank, world_size):
    print(f"Running FSDP checkpoint saving example on rank {rank}.")
    setup(rank, world_size)

    # create a pre-trained Vision Transformer (Huge) model and move it to GPU
    # The vit_h_14 model has over 600 million parameters.
    model = models.vit_h_14(weights=models.ViT_H_14_Weights.DEFAULT).to(rank)
    model = fully_shard(model)

    # Adjust the optimizer to use the model's parameters
    optimizer = torch.optim.Adam(model.parameters(), lr=0.001)

    # Create a dummy input tensor that ViT-H/14 expects (batch_size, channels, height, width)
    # The default input size for this specific model variant is 518x518.
    dummy_input = torch.rand(2, 3, 518, 518, device="cuda")

    start_time = time.time()

    checkpoint_future = None
    for step in range(10):
        optimizer.zero_grad()
        model(dummy_input).sum().backward()
        optimizer.step()

        # waits for checkpointing to finish if one exists, avoiding queuing more then one checkpoint request at a time
        if checkpoint_future is not None:
            checkpoint_future.result()

        state_dict = { "app": AppState(model, optimizer) }
        checkpoint_future = dcp.async_save(state_dict, checkpoint_id=f"{CHECKPOINT_DIR}_step{step}")

    end_time = time.time()
    elapsed_time = end_time - start_time

    print(f"Rank {rank}: {elapsed_time:.4f} seconds to run")

    cleanup()


if __name__ == "__main__":
    world_size = torch.cuda.device_count()
    print(f"Running async checkpoint example on {world_size} devices.")
    mp.spawn(
        run_fsdp_checkpoint_save_example,
        args=(world_size,),
        nprocs=world_size,
        join=True,
    )
