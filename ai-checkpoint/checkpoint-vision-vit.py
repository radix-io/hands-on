import os
import time

import torch
import torch.distributed as dist
import torch.multiprocessing as mp
import torch.nn as nn
import torchvision.models as models

from torch.nn.parallel import DistributedDataParallel as DDP

CHECKPOINT_DIR = "checkpoint/vis"


def setup(rank, world_size):
    # initialize the process group
    dist.init_process_group("nccl", rank=rank, world_size=world_size)
    torch.cuda.set_device(rank)


def cleanup():
    dist.destroy_process_group()


def run_ddp_checkpoint_save_example(rank, world_size):
    print(f"Running DDP checkpoint saving example on rank {rank}.")
    setup(rank, world_size)

    # create a pre-trained Vision Transformer (Huge) model and move it to GPU
    model = models.vit_h_14(weights=models.ViT_H_14_Weights.DEFAULT).to(rank)

    # Wrap the model with DDP
    model = DDP(model, device_ids=[rank])

    # Adjust the optimizer to use the model's parameters
    optimizer = torch.optim.Adam(model.parameters(), lr=0.001)

    # Create a dummy input tensor that ViT-H/14 expects
    dummy_input = torch.rand(2, 3, 518, 518, device="cuda")

    start_time = time.time()

    for step in range(10):
        optimizer.zero_grad()
        model(dummy_input).sum().backward()
        optimizer.step()

        # Save checkpoint only on rank 0
        if rank == 0:
            checkpoint_path = os.path.join(CHECKPOINT_DIR, f"checkpoint_step_{step}.pt")
            os.makedirs(CHECKPOINT_DIR, exist_ok=True)
            torch.save(
                {
                    "model_state_dict": model.module.state_dict(),
                    "optimizer_state_dict": optimizer.state_dict(),
                },
                checkpoint_path,
            )

    end_time = time.time()
    elapsed_time = end_time - start_time

    print(f"Rank {rank}: {elapsed_time:.4f} seconds to run")

    cleanup()


if __name__ == "__main__":
    world_size = torch.cuda.device_count()
    print(f"Running DDP checkpoint example on {world_size} devices.")
    mp.spawn(
        run_ddp_checkpoint_save_example,
        args=(world_size,),
        nprocs=world_size,
        join=True,
    )
