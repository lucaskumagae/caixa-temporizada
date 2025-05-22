<?php
include 'conexao.php';

$mensagem = "";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    if (isset($_POST['adicionar_configuracao'])) {
        $hora_inicio_np = $_POST['hora_inicio_np'];
        $hora_fim_np = $_POST['hora_fim_np'];
        $modo_nao_perturbe = isset($_POST['modo_nao_perturbe']) ? 1 : 0;

        $stmt = $conn->prepare("INSERT INTO configuracao (modo_nao_perturbe, hora_inicio_np, hora_fim_np) VALUES (?, ?, ?)");
        $stmt->bind_param("iss", $modo_nao_perturbe, $hora_inicio_np, $hora_fim_np);

        if ($stmt->execute()) {
            $mensagem = "Configuração adicionada com sucesso!";
        } else {
            $mensagem = "Erro ao adicionar configuração: " . $stmt->error;
        }

        $stmt->close();
    } elseif (isset($_POST['editar_configuracao'])) {
        $id_configuracao = $_POST['id_configuracao'];
        $hora_inicio_np = $_POST['hora_inicio_np'];
        $hora_fim_np = $_POST['hora_fim_np'];
        $modo_nao_perturbe = isset($_POST['modo_nao_perturbe']) ? 1 : 0;

        $stmt = $conn->prepare("UPDATE configuracao SET modo_nao_perturbe = ?, hora_inicio_np = ?, hora_fim_np = ? WHERE id_configuracao = ?");
        $stmt->bind_param("issi", $modo_nao_perturbe, $hora_inicio_np, $hora_fim_np, $id_configuracao);

        if ($stmt->execute()) {
            $mensagem = "Configuração atualizada com sucesso!";
        } else {
            $mensagem = "Erro ao atualizar configuração: " . $stmt->error;
        }

        $stmt->close();
    } elseif (isset($_POST['excluir_configuracao'])) {
        $id_configuracao = $_POST['id_configuracao'];

        $stmt = $conn->prepare("DELETE FROM configuracao WHERE id_configuracao = ?");
        $stmt->bind_param("i", $id_configuracao);

        if ($stmt->execute()) {
            $mensagem = "Configuração excluída com sucesso!";
        } else {
            $mensagem = "Erro ao excluir configuração: " . $stmt->error;
        }

        $stmt->close();
    } elseif (isset($_POST['adicionar_usuario'])) {
        $nome = $_POST['nome'];
        $email = $_POST['email'];

        $stmt = $conn->prepare("INSERT INTO usuario (nome, email) VALUES (?, ?)");
        $stmt->bind_param("ss", $nome, $email);

        if ($stmt->execute()) {
            $mensagem = "Usuário adicionado com sucesso!";
        } else {
            $mensagem = "Erro ao adicionar usuário: " . $stmt->error;
        }

        $stmt->close();
    } elseif (isset($_POST['editar_usuario'])) {
        $id_usuario = $_POST['id_usuario'];
        $nome = $_POST['nome'];
        $email = $_POST['email'];

        $stmt = $conn->prepare("UPDATE usuario SET nome = ?, email = ? WHERE id_usuario = ?");
        $stmt->bind_param("ssi", $nome, $email, $id_usuario);

        if ($stmt->execute()) {
            $mensagem = "Usuário atualizado com sucesso!";
        } else {
            $mensagem = "Erro ao atualizar usuário: " . $stmt->error;
        }

        $stmt->close();
    } elseif (isset($_POST['excluir_usuario'])) {
        $id_usuario = $_POST['id_usuario'];

        $stmt = $conn->prepare("DELETE FROM usuario WHERE id_usuario = ?");
        $stmt->bind_param("i", $id_usuario);

        if ($stmt->execute()) {
            $mensagem = "Usuário excluído com sucesso!";
        } else {
            $mensagem = "Erro ao excluir usuário: " . $stmt->error;
        }

        $stmt->close();
    }
}
$result_config = $conn->query("SELECT * FROM configuracao");
$configuracoes = $result_config->fetch_all(MYSQLI_ASSOC);

$result = $conn->query("SELECT * FROM usuario");
$usuarios = $result->fetch_all(MYSQLI_ASSOC);

$conn->close();
?>

<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Configurações</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
<?php include 'nav.php'; ?>

<main class="content">

    <section class="configuracao-usuario">
        <header class="header">
            <h1>Configurações de Usuário</h1>
        </header>

        <form method="post" class="form-adicionar-usuario">
            <input type="hidden" name="adicionar_usuario" value="1">
            <label for="nome">Nome:</label>
            <input type="text" id="nome" name="nome" required>
            <label for="email">Email:</label>
            <input type="email" id="email" name="email">
            <button type="submit">Adicionar Usuário</button>
        </form>

        <table>
            <thead>
                <tr>
                    <th>Nome</th>
                    <th>Email</th>
                    <th>Ações</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($usuarios as $usuario): ?>
                <tr>
                    <form method="post">
                        <input type="hidden" name="id_usuario" value="<?= $usuario['id_usuario'] ?>">
                        <td><input type="text" name="nome" value="<?= htmlspecialchars($usuario['nome']) ?>" required></td>
                        <td><input type="email" name="email" value="<?= htmlspecialchars($usuario['email']) ?>"></td>
                        <td>
                            <button type="submit" name="editar_usuario">Editar</button>
                            <button type="submit" name="excluir_usuario" onclick="return confirm('Tem certeza que deseja excluir este usuário?');">Excluir</button>
                        </td>
                    </form>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    </section>

    <section class="configuracao-tabela">
        <header class="header">
            <h1>Configurações do Modo Não Perturbe</h1>
        </header>

        <?php if (!empty($mensagem)) echo "<p><strong>$mensagem</strong></p>"; ?>

        <table>
            <thead>
                <tr>
                    <th>Horário de Início</th>
                    <th>Horário de Fim</th>
                    <th>Ativar Modo Não Perturbe</th>
                    <th>Ações</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($configuracoes as $configuracao): ?>
                <tr>
                    <form method="post">
                        <input type="hidden" name="id_configuracao" value="<?= $configuracao['id_configuracao'] ?>">
                        <td><input type="time" name="hora_inicio_np" value="<?= htmlspecialchars($configuracao['hora_inicio_np']) ?>" required></td>
                        <td><input type="time" name="hora_fim_np" value="<?= htmlspecialchars($configuracao['hora_fim_np']) ?>" required></td>
                        <td><input type="checkbox" name="modo_nao_perturbe" value="1" <?= $configuracao['modo_nao_perturbe'] ? 'checked' : '' ?>></td>
                        <td>
                            <button type="submit" name="editar_configuracao">Editar</button>
                            <button type="submit" name="excluir_configuracao" onclick="return confirm('Tem certeza que deseja excluir esta configuração?');">Excluir</button>
                        </td>
                    </form>
                </tr>
                <?php endforeach; ?>
                <tr>
                    <form method="post">
                        <td><input type="time" name="hora_inicio_np" required></td>
                        <td><input type="time" name="hora_fim_np" required></td>
                        <td><input type="checkbox" name="modo_nao_perturbe" value="1"></td>
                        <td><button type="submit" name="adicionar_configuracao">Adicionar</button></td>
                    </form>
                </tr>
            </tbody>
        </table>
    </section>

</main>
